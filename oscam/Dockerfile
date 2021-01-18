FROM alpine:latest
RUN apk add eudev
WORKDIR /etc/oscam
COPY *oscam*.* /etc/oscam/
COPY after_start /etc/oscam/
ENTRYPOINT ["/etc/oscam/after_start"]
