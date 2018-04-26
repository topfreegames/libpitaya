const net = require('net');
const tls = require('tls');
const fs = require('fs');
const pkt = require('./packet.js');

const HOST = '127.0.0.1';

const TCP_PORT = 4000;
const TLS_PORT = TCP_PORT+1;

let handshakeResponseData;
let heartbeatResponseData;
let heartbeatInterval;
let clientDisconnected = false;

function sendHeartbeat(socket) {
    console.log('sending heartbeat');
    socket.write(heartbeatResponseData);
}

function processPacket(packet, clientSocket) {
    console.log('processing packet');

    switch (packet.type) {
    case pkt.PacketType.Handshake:
        console.log('Sending handshake response... ', handshakeResponseData[0],
                    ' len: ', handshakeResponseData.length);
        clientSocket.write(handshakeResponseData);
        break;
        // a.SetStatus(constants.StatusHandshake)
        // logger.Log.Debugf("Session handshake Id=%d, Remote=%s", a.Session.ID(), a.RemoteAddr())

    case pkt.PacketType.HandshakeAck:
        console.log('RECEIVED HANDSHAKE ACK');

        if (clientDisconnected && !heartbeatInterval) {
            heartbeatInterval = setInterval(() => {
                if (clientSocket && !clientSocket.destroyed) {
                    sendHeartbeat(clientSocket);
                }
            }, 2000);
        }
        break;

    case pkt.PacketType.Data:
        console.log('DATA PACKAGE BOI');
        // if a.GetStatus() < constants.StatusWorking {
            // return fmt.Errorf("receive data on socket which is not yet ACK, session will be closed immediately, remote=%s",
                                // a.RemoteAddr().String())
        // }

        // TODO: decode message
        // msg, err := message.Decode(p.Data)
        // if err != nil {
        //     return err
        // }
        // TODO: process message
        // h.processMessage(a, msg)
        break;

    case pkt.PacketType.Heartbeat:
        // expected
        break;
    }

    // TODO: Return an error here
    // a.SetLastAt()
    // return nil
}

(function encodeHanshakeResponse() {
    // Hardcoded handshake data
    const hData = {
        'code': 200,
        'sys': {
            'heartbeat': 2, // 2 seconds,
            'dict': {
		            'connector.getsessiondata': 1,
		            'connector.setsessiondata': 2,
		            'room.room.getsessiondata': 3,
		            'onMessage':                4,
		            'onMembers':                5,
            }
        }
    };
    const data = JSON.stringify(hData);
    console.log(data);

    handshakeResponseData = pkt.encode(pkt.PacketType.Handshake, Buffer.from(data));
    heartbeatResponseData = pkt.encode(pkt.PacketType.Heartbeat);

    console.log('Handshake response data: ', handshakeResponseData);
})();

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
        processBuffer(buffer, socket);
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
        processBuffer(buffer, socket);
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
