FROM alpine:edge
RUN apk add --update alpine-sdk jansson-dev

RUN wget https://github.com/fredrikwidlund/libdynamic/releases/download/v1.3.0/libdynamic-1.3.0.tar.gz && \
    tar fvxz libdynamic-1.3.0.tar.gz && \
    (cd libdynamic-1.3.0/ && ./configure --prefix=/usr AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib && make install)

RUN wget https://github.com/fredrikwidlund/libreactor/releases/download/v1.0.1/libreactor-1.0.1.tar.gz && \
        tar fvxz libreactor-1.0.1.tar.gz && \
        (cd libreactor-1.0.1; ./configure --prefix=/usr AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib; make install)
