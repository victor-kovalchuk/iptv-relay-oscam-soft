FROM alpine:latest
WORKDIR /etc/astra
COPY astra-alpine /etc/astra/
COPY relay_new.lua /etc/astra/
ENTRYPOINT /etc/astra/astra-alpine --relay /etc/astra/relay_new.lua -p 8888 --buffer-size 8192 --buffer-fill 128
