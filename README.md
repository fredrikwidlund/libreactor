libreactor
==========

Extendable event driven high performance C-abstractions

Installation
------------

    ./autogen.sh
    ./configure
    make
    sudo make install

Tests
-----

Requires cmocka (http://cmocka.org/) to be installed, as well as valgrind (http://valgrind.org/) for memory tests.

    make check
