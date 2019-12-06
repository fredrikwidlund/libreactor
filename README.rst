=================
libreactor v1.0.1
=================

.. image:: https://travis-ci.org/fredrikwidlund/libreactor.svg?branch=master
  :target: https://travis-ci.org/fredrikwidlund/libreactor
    
.. image:: https://coveralls.io/repos/github/fredrikwidlund/libreactor/badge.svg?branch=master
  :target: https://coveralls.io/github/fredrikwidlund/libreactor?branch=master

* is a modular event driven application framework written in C
* is an extendable generic framework that can be used for web applications, and many other things
* comes with a very low overhead
* is fast (pretty much as fast as it gets)
* has been production stable for several years serving tens of millions of requests per day
* has high level abstractions like ``reactor_server``, CouchDB integration and worker pools

--------
Building
--------

The framework depends on libdynamic_ and libjansson_ so please install these first.

.. code-block:: shell

    apt-get install -y build-essential libjansson-dev wget
    wget https://github.com/fredrikwidlund/libdynamic/releases/download/v1.3.0/libdynamic-1.3.0.tar.gz
    tar fvxz libdynamic-1.3.0.tar.gz
    (cd libdynamic-1.3.0; ./configure --prefix=/usr AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib; make install)
    wget https://github.com/fredrikwidlund/libreactor/releases/download/v1.0.0/libreactor-1.0.1.tar.gz
    tar fvxz libreactor-1.0.1.tar.gz
    (cd libreactor-1.0.1; ./configure --prefix=/usr AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib; make install)

--------
Examples
--------

Hello world web server
=================

.. code-block:: C

    $ cat example/hello.c 
    #include <stdio.h>
    #include <stdint.h>
    #include <unistd.h>
    #include <dynamic.h>
    #include <reactor.h>
    
    static reactor_status hello(reactor_event *event)
    {
      reactor_server_session *session = (reactor_server_session *) event->data;
      reactor_server_ok(session, reactor_vector_string("text/plain"), reactor_vector_string("Hello, World!"));
      return REACTOR_OK;
    }
    
    int main()
    {
      reactor_server server;
      reactor_construct();
      reactor_server_construct(&server, NULL, NULL);
      reactor_server_route(&server, hello, NULL);
      (void) reactor_server_open(&server, "0.0.0.0", "8080");
      reactor_run();
      reactor_destruct();
    }
    $ gcc -Wall -Wpedantic -std=c11 -O2 -o hello hello.c -lreactor -ldynamic -pthread -flto

----------
Try it out
----------

The following will build a portable static binary ``example/hello`` (~90kB) for the above example code

.. code-block:: shell

    wget https://github.com/fredrikwidlund/libreactor/releases/download/v1.0.0/libreactor-1.0.1.tar.gz
    tar fvxz libreactor-1.0.1.tar.gz
    cd libreactor-1.0.1
    ./configure --prefix=/usr AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib
    make hello

Run it

.. code-block:: shell

    ./example/hello &
    curl localhost:8080

.. _libdynamic: https://github.com/fredrikwidlund/libdynamic
.. _libjansson: https://github.com/akheron/jansson
