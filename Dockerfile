FROM alpine

WORKDIR /app
ADD ./out/json_server_linux ./json_server
ADD ./out/protobuf_server_linux ./protobuf_server

CMD ./json_server --port 4000 && ./protobuf_server --port 5000


