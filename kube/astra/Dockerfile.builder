FROM alpine:latest
RUN apk add musl-dev linux-headers git gcc make
RUN mkdir /usr/src
RUN git clone https://github.com/victor-kovalchuk/astra.git /usr/src/astra
WORKDIR /usr/src/astra
RUN ./configure.sh --with-libdvbcsa && make
CMD ["/bin/sh"]

