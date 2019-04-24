FROM alpine

WORKDIR /app
ADD ./out/json_server_linux ./json_server
ADD ./fixtures ./fixtures

EXPOSE 3250
EXPOSE 3251
EXPOSE 3252

CMD /app/json_server -port 3250

