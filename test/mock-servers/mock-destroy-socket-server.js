const net = require('net');
const tls = require('tls');
const fs = require('fs');
const pkt = require('./packet.js');
const message = require('./message.js');

const HOST = '127.0.0.1';
const TCP_PORT = 4400;
const TLS_PORT = TCP_PORT+1;
const HEARTBEAT_INTERVAL = 6;

let heartbeatInterval;
let clientDisconnected = false;

const tcpServer = net.createServer((socket) => {
    console.log('======= New TCP Connection ========');

    socket.on('data', (buffer) => {
        setTimeout(() => socket.destroy(), 2000);
    });

    socket.on('end', () => {
        clientDisconnected = true;
        console.log('Client disconnected :(');
    });
});


const tlsOptions = {
    key: fs.readFileSync('../server/fixtures/server/client-ssl.localhost.key'),
    cert: fs.readFileSync('../server/fixtures/server/client-ssl.localhost.crt'),
    rejectUnauthorized: false,
};

const tlsServer = tls.createServer(tlsOptions, (socket) => {
    console.log('======= New TLS Connection ========');
    console.log(socket.authorized ? 'Authorized' : 'Unauthorized');

    socket.on('data', (buffer) => {
        setTimeout(() => socket.destroy(), 2000);
    });

    socket.on('end', () => {
        clientDisconnected = true;
        console.log('Client disconnected :(');
    });
});

tcpServer.listen(TCP_PORT, HOST, () => {
    console.log(`TCP server on ${HOST}:${TCP_PORT}`);
});

tlsServer.listen(TLS_PORT, HOST, () => {
    console.log(`TLS server on ${HOST}:${TLS_PORT}`);
});

pkt.encodeHanshakeAndHeartbeatResponse(HEARTBEAT_INTERVAL);
