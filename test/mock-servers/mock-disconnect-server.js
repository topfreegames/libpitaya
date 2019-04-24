const net = require('net');
const tls = require('tls');
const fs = require('fs');
const pkt = require('./packet.js');

const HOST = '127.0.0.1';
const TCP_PORT = 4000;
const TLS_PORT = TCP_PORT+1;
const HEARTBEAT_INTERVAL = 2;

let heartbeatInterval;
let clientDisconnected = false;

function processPacket(packet, clientSocket) {
    console.log('processing packet');

    switch (packet.type) {
    case pkt.PacketType.Handshake:
        pkt.sendHandshakeResponse(clientSocket);
        break;

    case pkt.PacketType.HandshakeAck:
        console.log('RECEIVED HANDSHAKE ACK');

        if (clientDisconnected && !heartbeatInterval) {
            heartbeatInterval = setInterval(() => {
                if (clientSocket && !clientSocket.destroyed) {
                    pkt.sendHeartbeat(clientSocket);
                }
            }, 2000);
        }
        break;

    case pkt.PacketType.Data: /* Ignore */ break;
    case pkt.PacketType.Heartbeat: /* expected */ break;
    }
}

function processBuffer(buffer, socket) {
    console.log(`Received ${buffer.length} bytes of data`);
    console.log('Decoding packets...');
    const rawPackets = new pkt.RawPackets(buffer);
    const packets = rawPackets.decode();

    console.log(`Decoded ${packets.length} packet(s)`);

    for (let p of packets) {
        processPacket(p, socket);
    }
}

const tcpServer = net.createServer((socket) => {
    console.log('======= New TCP Connection ========');

    socket.on('data', (buffer) => {
        clientDisconnected = false;
        processBuffer(buffer, socket);
    });

    socket.on('end', () => {
        clientDisconnected = true;
        console.log('Client disconnected :(');
    });

    socket.on('error', () => {
        clientDisconnected = true;
        clearTimeout(handshakeTimeout);
        console.log('Client disconnected with error :(');
    });
});

const tlsOptions = {
    key: fs.readFileSync('../../fixtures/server/pitaya.key'),
    cert: fs.readFileSync('../../fixtures/server/pitaya.crt'),
    rejectUnauthorized: false,
};

const tlsServer = tls.createServer(tlsOptions, (socket) => {
    console.log('======= New TLS Connection ========');
    console.log(socket.authorized ? 'Authorized' : 'Unauthorized');

    socket.on('data', (buffer) => {
        clientDisconnected = false;
        processBuffer(buffer, socket);
    });

    socket.on('end', () => {
        clientDisconnected = true;
        console.log('Client disconnected :(');
    });

    socket.on('error', () => {
        clientDisconnected = true;
        clearTimeout(handshakeTimeout);
        console.log('Client disconnected with error :(');
    });
});

tcpServer.listen(TCP_PORT, HOST, () => {
    console.log(`TCP server on ${HOST}:${TCP_PORT}`);
});

tlsServer.listen(TLS_PORT, HOST, () => {
    console.log(`TLS server on ${HOST}:${TLS_PORT}`);
});

pkt.encodeHanshakeAndHeartbeatResponse(HEARTBEAT_INTERVAL);
