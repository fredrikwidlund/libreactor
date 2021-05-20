=================
libreactor v2.0.0
=================

.. image:: https://coveralls.io/repos/github/fredrikwidlund/libreactor/badge.svg?branch=master
  :target: https://coveralls.io/github/fredrikwidlund/libreactor?branch=master

----------
Try it out
----------

The following will build a portable static web server ``hello`` (~60kB)

.. code-block:: shell

    git clone https://github.com/fredrikwidlund/libreactor.git
    cd libreactor/
    ./autogen.sh
    ./configure
    make hello

Run it

.. code-block:: shell

    ./hello

In another shell try

.. code-block:: shell

    wrk http://localhost

.. _libdynamic: https://github.com/fredrikwidlund/libdynamic
