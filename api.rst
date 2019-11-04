*************
API Reference
*************

Design
======

.. code:: C

          reactor_status  reactor_construct(void);
          reactor_status  reactor_run(void);
          void            reactor_destruct(void);
          
          reactor_status  reactor_core_add(reactor_user *, int, int);
          reactor_status  reactor_core_modify(reactor_user *, int, int);
          void            reactor_core_delete(reactor_user *, int);
          
          reactor_status  reactor_pool_dispatch(reactor_user_callback *, void *);
          reactor_status  reactor_resolver_request(reactor_user_callback *, void *, char *, char *, int, int, int);
          
          void            reactor_fd_construct(reactor_socket *, reactor_user_callback *, void *);
          void            reactor_fd_destruct(reactor_socket *);
          int             reactor_fd_deconstruct(reactor_fd *);
          reactor_status  reactor_fd_open(reactor_fd *, int, int);
          void            reactor_fd_close(reactor_fd *);
          reactor_status  reactor_fd_events(reactor_fd *, int);
          int             reactor_fd_active(reactor_fd *);
          int             reactor_fd_fileno(reactor_fd *);
          
          void            reactor_timer_construct(reactor_timer *, reactor_user_callback *, void *);
          void            reactor_timer_destruct(reactor_timer *);
          reactor_status  reactor_timer_set(reactor_timer *, uint64_t, uint64_t);
          void            reactor_timer_clear(reactor_timer *);
          
          void            reactor_stream_construct(reactor_stream *, reactor_user_callback *, void *);
          void            reactor_stream_user(reactor_stream *, reactor_user_callback *, void *);
          void            reactor_stream_destruct(reactor_stream *);
          void            reactor_stream_reset(reactor_stream *);
          reactor_status  reactor_stream_open(reactor_stream *, int);
          void           *reactor_stream_data(reactor_stream *);
          size_t          reactor_stream_size(reactor_stream *);
          void            reactor_stream_consume(reactor_stream *, size_t);
          void           *reactor_stream_segment(reactor_stream *, size_t);
          void            reactor_stream_write(reactor_stream *, void *, size_t);
          reactor_status  reactor_stream_flush(reactor_stream *);
          
          void            reactor_net_construct(reactor_net *, reactor_user_callback *, void *);
          void            reactor_net_destruct(reactor_net *);
          void            reactor_net_reset(reactor_net *);
          void            reactor_net_set(reactor_net *, reactor_net_options);
          void            reactor_net_clear(reactor_net *, reactor_net_options);
          reactor_status  reactor_net_bind(reactor_net *, char *, char *);
          reactor_status  reactor_net_connect(reactor_net *, char *, char *);

          reactor_vector reactor_http_headers_lookup(reactor_http_headers *, reactor_vector);
          int            reactor_http_headers_match(reactor_http_headers *, reactor_vector, reactor_vector);
          void           reactor_http_headers_add(reactor_http_headers *, reactor_vector, reactor_vector);

          void           reactor_http_request_construct(reactor_http_request *, reactor_vector, reactor_vector, int, reactor_vector);

          void           reactor_http_construct(reactor_http *, reactor_user_callback *, void *);
          void           reactor_http_destruct(reactor_http *);
          reactor_status reactor_http_open(reactor_http *, int);
          reactor_status reactor_http_flush(reactor_http *);
          void           reactor_http_reset(reactor_http *);
          void           reactor_http_set_authority(reactor_http *, reactor_vector, reactor_vector);
          void           reactor_http_set_mode(reactor_http *, reactor_http_mode);
          void           reactor_http_create_request(reactor_http *, reactor_http_request *, reactor_vector, reactor_vector, int, reactor_vector, size_t, reactor_vector);
          void           reactor_http_write_request(reactor_http *, reactor_http_request *);
          void           reactor_http_create_response(reactor_http *, reactor_http_response *, int, int, reactor_vector, reactor_vector, size_t, reactor_vector);
          void           reactor_http_write_response(reactor_http *, reactor_http_response *);

          void           reactor_http_get(reactor_http *, reactor_vector);

          void           reactor_server_construct(reactor_server *, reactor_user_callback *, void *);
          void           reactor_server_destruct(reactor_server *);
          reactor_status reactor_server_open(reactor_server *, char *, char *);
          void           reactor_server_route(reactor_server *, reactor_user_callback *, void *);
          void           reactor_server_close(reactor_server_session *);
          void           reactor_server_register(reactor_server_session *, reactor_user_callback *, void *);
          void           reactor_server_respond(reactor_server_session *, reactor_http_response *);
          void           reactor_server_ok(reactor_server_session *, reactor_vector, reactor_vector);
          void           reactor_server_found(reactor_server_session *, reactor_vector);
          void           reactor_server_not_found(reactor_server_session *);
