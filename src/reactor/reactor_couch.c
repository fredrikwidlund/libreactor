#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <jansson.h>
#include <dynamic.h>

#include "../reactor.h"

static reactor_status reactor_couch_set_status(reactor_couch *couch, reactor_couch_status status)
{
  if (couch->status == status)
    return REACTOR_OK;

  couch->status = status;
  return reactor_user_dispatch(&couch->user, REACTOR_COUCH_EVENT_STATUS, (uintptr_t) couch->status);
}

static reactor_status reactor_couch_reset(reactor_couch *couch)
{
  reactor_net_reset(&couch->net);
  reactor_http_reset(&couch->http);
  couch->state = REACTOR_COUCH_STATE_DISCONNECTED;
  return REACTOR_ABORT;
}

static reactor_status reactor_couch_error(reactor_couch *couch, char *description)
{
  return reactor_user_dispatch(&couch->user, REACTOR_COUCH_EVENT_ERROR, (uintptr_t) description);
}

static reactor_status reactor_couch_warning(reactor_couch *couch, char *description)
{
  return reactor_user_dispatch(&couch->user, REACTOR_COUCH_EVENT_WARNING, (uintptr_t) description);
}

static reactor_status reactor_couch_retry(reactor_couch *couch, char *description)
{
  reactor_status e;

  e = reactor_couch_set_status(couch, REACTOR_COUCH_STATUS_STALE);
  if (e != REACTOR_OK)
    return e;

  e = reactor_couch_warning(couch, description);
  if (e != REACTOR_OK)
    return e;

  return reactor_couch_reset(couch);
}

static int reactor_couch_line(reactor_vector *data, reactor_vector *line)
{
  char *eol;
  size_t n;

  eol = memchr(data->base, '\n', data->size);
  if (!eol)
    return 0;
  n = eol - (char *) data->base;

  line->base = data->base;
  line->size = n;
  data->base = (char *) data->base + n + 1;
  data->size -= n + 1;
  return 1;
}

static int reactor_couch_sync(char *seq, char *update_seq)
{
  return atoi(seq) >= atoi(update_seq);
}

static reactor_status reactor_couch_connect(reactor_couch *couch)
{
  reactor_status e;

  e = reactor_net_connect(&couch->net, couch->node, couch->service);
  if (e != REACTOR_OK)
    return reactor_couch_error(couch, "error connecting to database");

  couch->state = REACTOR_COUCH_STATE_CONNECTING;
  return REACTOR_OK;
}

static reactor_status reactor_couch_update_seq(reactor_couch *couch, int fd)
{
  char path[4096];

  (void) reactor_http_open(&couch->http, fd);
  snprintf(path, sizeof path, "/%s", couch->database);
  reactor_http_set_authority(&couch->http, reactor_vector_string(couch->node), reactor_vector_string(couch->service));
  reactor_http_get(&couch->http, reactor_vector_string(path));

  couch->state = REACTOR_COUCH_STATE_UPDATE_SEQ;
  return REACTOR_OK;
}

static reactor_status reactor_couch_subscribe(reactor_couch *couch, reactor_http_response *response)
{
  char path[4096];
  const char *s;
  json_t *v;

  if (response->code != 200)
    return reactor_couch_retry(couch, "database not found");

  v = json_loadb(response->body.base, response->body.size, 0, NULL);
  if (!v)
    return reactor_couch_retry(couch, "invalid reply");

  s = json_string_value(json_object_get(v, "update_seq"));
  if (!s)
    return reactor_couch_retry(couch, "no database update sequence value");
  free(couch->update_seq);
  couch->update_seq = strdup(s);
  if (!couch->update_seq)
    abort();
  json_decref(v);

  (void) snprintf(path, sizeof path, "/%s/_changes?include_docs=true&feed=continuous&since=%s&heartbeat=1000",
                  couch->database, couch->seq ? couch->seq : "0");

  reactor_http_set_mode(&couch->http, REACTOR_HTTP_MODE_RESPONSE_STREAM);
  reactor_http_get(&couch->http, reactor_vector_string(path));
  couch->state = REACTOR_COUCH_STATE_SUBSCRIBE;
  return REACTOR_OK;
}

static reactor_status reactor_couch_receive(reactor_couch *couch, reactor_http_response *response)
{
  reactor_status e;

  if (response->code != 200)
    return reactor_couch_retry(couch, "subscription failed");

  couch->heartbeat = reactor_core_now();
  couch->state = REACTOR_COUCH_STATE_VALUE;

  if (reactor_couch_sync(couch->seq ? couch->seq : "0", couch->update_seq))
    e = reactor_couch_set_status(couch, REACTOR_COUCH_STATUS_ONLINE);
  else
    e = reactor_couch_set_status(couch, REACTOR_COUCH_STATUS_STALE);
  if (e != REACTOR_OK)
    return e;

  return REACTOR_OK;
}

static reactor_status reactor_couch_value(reactor_couch *couch, reactor_vector *data)
{
  reactor_vector line;
  reactor_status e;
  char *seq, *id;
  json_t *v, *doc;
  json_error_t error;
  int n;

  if (data->size == 0)
    return reactor_couch_retry(couch, "subscription ended");

  if (data->size == 1)
    {
      couch->heartbeat = reactor_core_now();
      return REACTOR_OK;
    }

  while (data->size)
    {
      n = reactor_couch_line(data, &line);
      if (!n)
        break;

      v = json_loadb(line.base, line.size, 0, &error);
      if (!v)
        return reactor_couch_retry(couch, "unable to parse value");

      doc = json_object_get(v, "doc");
      if (!doc)
        return reactor_couch_retry(couch, "entry without document");

      seq = (char *) json_string_value(json_object_get(v, "seq"));
      if (!seq)
        return reactor_couch_retry(couch, "entry without sequence number");
      free(couch->seq);
      couch->seq = strdup(seq);

      id = (char *) json_string_value(json_object_get(doc, "_id"));
      if (id && couch->id && strcmp(id, couch->id) != 0)
        e = REACTOR_OK;
      else
        e = reactor_user_dispatch(&couch->user, REACTOR_COUCH_EVENT_UPDATE, (uintptr_t) doc);
      json_decref(v);
      if (e != REACTOR_OK)
        return e;

      if (couch->status == REACTOR_COUCH_STATUS_STALE && reactor_couch_sync(couch->seq, couch->update_seq))
        {
          e = reactor_couch_set_status(couch, REACTOR_COUCH_STATUS_ONLINE);
          if (e != REACTOR_OK)
            return e;
        }
    }

  if (data->size)
    return reactor_couch_retry(couch, "invalid value data");

  return REACTOR_OK;
}

static reactor_status reactor_couch_timer_handler(reactor_event *event)
{
  reactor_couch *couch = event->state;

  switch (event->type)
    {
    case REACTOR_TIMER_EVENT_ALARM:
      if (couch->state == REACTOR_COUCH_STATE_DISCONNECTED)
        return reactor_couch_connect(couch);
      if (couch->state == REACTOR_COUCH_STATE_VALUE &&
          reactor_core_now() - couch->heartbeat > 20000000000)
        return reactor_couch_retry(couch, "no heartbeat");
      break;
    default:
      return reactor_couch_error(couch, "timer error");
    }
  return REACTOR_OK;
}

static reactor_status reactor_couch_net_handler(reactor_event *event)
{
  reactor_couch *couch = event->state;

  switch (event->type)
    {
    case REACTOR_NET_EVENT_ERROR:
      return reactor_couch_retry(couch, "unable to connect to database");
    case REACTOR_NET_EVENT_CONNECT:
      return reactor_couch_update_seq(couch, event->data);
    default:
      return reactor_couch_retry(couch, "invalid network event");
    }
  return REACTOR_OK;
}

static reactor_status reactor_couch_http_handler(reactor_event *event)
{
  reactor_couch *couch = (reactor_couch *) event->state;

  switch (event->type)
    {
    case REACTOR_HTTP_EVENT_RESPONSE:
       if (couch->state == REACTOR_COUCH_STATE_UPDATE_SEQ)
        return reactor_couch_subscribe(couch, (reactor_http_response *) event->data);
       return REACTOR_ABORT;
    case REACTOR_HTTP_EVENT_RESPONSE_HEAD:
      if (couch->state == REACTOR_COUCH_STATE_SUBSCRIBE)
        return reactor_couch_receive(couch, (reactor_http_response *) event->data);
      return REACTOR_ABORT;
    case REACTOR_HTTP_EVENT_RESPONSE_BODY:
      if (couch->state == REACTOR_COUCH_STATE_VALUE)
        return reactor_couch_value(couch, (reactor_vector *) event->data);
      return REACTOR_ABORT;
    case REACTOR_HTTP_EVENT_CLOSE:
      return reactor_couch_retry(couch, "couch connection closed");
    default:
    case REACTOR_HTTP_EVENT_ERROR:
      return reactor_couch_retry(couch, "http error");
    }
}

static reactor_status reactor_couch_location(reactor_couch *couch, char *location)
{
  char *p;

  if (!location)
    return REACTOR_ERROR;

  couch->location = strdup(location);
  if (!couch->location)
    abort();
  p = couch->location;

  if (strncmp(p, "http://", 7) == 0)
    p += 7;
  couch->node = p;

  p = strchr(couch->node, '/');
  if (!p)
    return REACTOR_ERROR;
  *p = 0;
  p ++;
  couch->database = p;
  if (*p == 0)
    return REACTOR_ERROR;

  couch->id = NULL;
  p = strchr(couch->database, '/');
  if (p)
    {
      *p = 0;
      p ++;
      if (*p)
        couch->id = p;
    }

  p = strchr(couch->node, ':');
  if (!p)
    couch->service = "5984";
  else
    {
      *p = 0;
      p ++;
      couch->service = p;
    }
  if (couch->node[0] == 0)
    couch->node = "127.0.0.1";

  return REACTOR_OK;
}

void reactor_couch_construct(reactor_couch *couch, reactor_user_callback *callback, void *state)
{
  *couch = (reactor_couch) {.status = REACTOR_COUCH_STATUS_OFFLINE};
  reactor_user_construct(&couch->user, callback, state);
  reactor_timer_construct(&couch->timer, reactor_couch_timer_handler, couch);
  reactor_net_construct(&couch->net, reactor_couch_net_handler, couch);
  reactor_http_construct(&couch->http, reactor_couch_http_handler, couch);
}

void reactor_couch_destruct(reactor_couch *couch)
{
  free(couch->location);
  free(couch->seq);
  free(couch->update_seq);
  reactor_http_destruct(&couch->http);
  reactor_net_destruct(&couch->net);
  reactor_timer_destruct(&couch->timer);
}

reactor_status reactor_couch_open(reactor_couch *couch, char *location)
{
  reactor_status e;

  e = reactor_couch_location(couch, location);
  if (e != REACTOR_OK)
    return e;

  couch->state = REACTOR_COUCH_STATE_DISCONNECTED;
  reactor_timer_set(&couch->timer, 0, 1000000000);

  return REACTOR_OK;
}

int reactor_couch_online(reactor_couch *couch)
{
  return couch->status == REACTOR_COUCH_STATUS_ONLINE;
}
