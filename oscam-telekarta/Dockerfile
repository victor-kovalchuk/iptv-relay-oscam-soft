FROM alpine:latest
RUN apk add eudev libstdc++ libc6-compat
WORKDIR /etc/oscam
COPY *oscam*.* /etc/oscam/
COPY oscam /etc/oscam/
COPY influx_write /etc/oscam/
COPY after_start /etc/oscam/
ENTRYPOINT ["/etc/oscam/after_start"]
