FROM alpine

WORKDIR /app
ADD ./out/protobuf_server_linux ./protobuf_server
ADD ./fixtures ./fixtures

EXPOSE 3350
EXPOSE 3351
EXPOSE 3352

CMD /app/protobuf_server -port 3350

