FROM alpine:edge
RUN apk add --update alpine-sdk linux-headers

RUN wget https://github.com/fredrikwidlund/libdynamic/releases/download/v2.3.0/libdynamic-2.3.0.tar.gz && \
    tar fvxz libdynamic-2.3.0.tar.gz && \
    (cd libdynamic-2.3.0/ && ./configure && make install)

RUN wget https://github.com/fredrikwidlund/libreactor/releases/download/v2.0.0-alpha/libreactor-2.0.0.tar.gz && \
    tar fvxz libreactor-2.0.0.tar.gz && \
    (cd libreactor-2.0.0; ./configure && make install)
