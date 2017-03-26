.. _apireference:

*************
API Reference
*************

.. highlight:: c

Library Version
===============

The libreactor version uses `Semantic Versioning`_ and is of the form *A.B.C*, where *A* is the major version, *B* is
the minor version and *C* is the patch version.

When a new release only fixes bugs and doesn't add new features or functionality, the patch version is incremented.
When new features are added in a backwards compatible way, the minor version is incremented and the micro version is
set to zero. When there are backwards incompatible changes, the major version is incremented and others are set to
zero.

The following preprocessor constants specify the current version of the library:

``LIBREACTOR_VERSION_MAJOR``, ``LIBREACTOR_VERSION_MINOR``, ``LIBREACTOR_VERSION_PATCH``
  Integers specifying the major, minor and patch versions, respectively.

``LIBREACTOR_VERSION``
  A string representation of the current version, e.g. ``"1.2.1"``

reactor_core
============

reactor_core is the main event loop object, and has low level interfaces to handle file descriptor events.

.. type:: reactor_core

  This private data structure represents the main event loop object.

.. function:: void reactor_core_construct()

  Constructs a thread local reactor_core object singleton.

.. function:: void reactor_core_destruct()

  Destructs a thread local reactor_core object singleton.

.. function:: void reactor_core_register(int fd, reactor_user_callback *callback, void *state, int events)

  Register *fd* in the reactor_core. Events specified in the *events* mask will trigger the *callback*
  function with *state* included as argument.

.. function:: void reactor_core_deregister(int fd)

  Deregister *fd* from the reactor_core.

.. function:: void *reactor_core_poll(int fd)

  Returns a pointer to the pollfd structure representing the *fd*.

.. function:: void *reactor_core_user(int fd)

  Returns a pointer to the reactor_user structure representing the *fd*.
              
.. function::  int reactor_core_run()

  Initiates the reactor_core event loop.

.. _`Semantic Versioning`: http://semver.org/

