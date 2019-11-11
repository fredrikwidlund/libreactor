libreactor
======

.. image:: https://travis-ci.org/fredrikwidlund/libreactor.svg?branch=master
  :target: https://travis-ci.org/fredrikwidlund/libreactor
    
.. image:: https://coveralls.io/repos/github/fredrikwidlund/libreactor/badge.svg?branch=master
  :target: https://coveralls.io/github/fredrikwidlund/libreactor?branch=master

Building
========

The framework depends on libdynamic_ and libjansson_ so please install these first.

.. code-block:: shell

    $ apt-get install -y build-essential libjansson-dev wget
    $ wget https://github.com/fredrikwidlund/libdynamic/releases/download/v1.3.0/libdynamic-1.3.0.tar.gz
    $ tar fvxz libdynamic-1.3.0.tar.gz
    $ (cd libdynamic-1.3.0; ./configure --prefix=/usr AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib; make install)
    $ wget https://github.com/fredrikwidlund/libreactor/releases/download/v1.0.0/libreactor-1.0.0.tar.gz
    $ tar fvxz libreactor-1.0.0.tar.gz
    $ (cd libreactor-1.0.0; ./configure --prefix=/usr AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib; make install)

.. _libdynamic: https://github.com/fredrikwidlund/libdynamic
.. _libjansson: https://github.com/akheron/jansson
