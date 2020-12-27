=======================
libreactor v2.0.0-alpha
=======================

.. image:: https://travis-ci.org/fredrikwidlund/libreactor.svg?branch=master
  :target: https://travis-ci.org/fredrikwidlund/libreactor
    
.. image:: https://coveralls.io/repos/github/fredrikwidlund/libreactor/badge.svg?branch=master
  :target: https://coveralls.io/github/fredrikwidlund/libreactor?branch=master

----------
Try it out
----------

The following will build a portable static web server ``hello`` (~60kB)

.. code-block:: shell

    git clone -b release-2.0 https://github.com/fredrikwidlund/libreactor.git
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
