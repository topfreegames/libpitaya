const net = require('net');

const HOST = '127.0.0.1';
const PORT = 4000;

let clientSocket = null;

// Constants for the header
const PacketType = Object.freeze({
    Handshake: 0x01,
    HandshakeAck: 0x02,
    Heartbeat: 0x03,
    Data: 0x04,
    Kick: 0x05,
});

const HEADER_LENGTH = 4;
const MAX_PACKET_SIZE = 64 * 1024;

function encode(packetType, data) {
    if (packetType < PacketType.Handshake || packetType > PacketType.Kick) {
        throw new Error(`Trying to encode invalid packet type ${packetType}`);
    }

    const headerBuf = Buffer.alloc(HEADER_LENGTH);
    headerBuf.writeUInt8(packetType, 0);
    headerBuf.writeIntBE(data ? data.length : 0, 1, 3);

    console.log('encoded packet type ', headerBuf[0]);
    console.log('encoded packet length ', headerBuf.readIntBE(1, 3));

    if (data) {
        console.log('data length ', data.length);
        return Buffer.concat([headerBuf, data], HEADER_LENGTH + data.length);
    } else {
        return headerBuf;
    }
}

let handshakeResponseData;
let heartbeatResponseData;
let heartbeatInterval;
let clientDisconnected = false;

function sendHeartbeat(socket) {
    console.log('sending heartbeat');
    socket.write(heartbeatResponseData);
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

    handshakeResponseData = encode(PacketType.Handshake, Buffer.from(data));
    heartbeatResponseData = encode(PacketType.Heartbeat);

    console.log('Handshake response data: ', handshakeResponseData);
})();

class Packet {
    constructor(type, length, data) {
        this.type = type;
        this.length = length;
        this.data = data;
    }

    //
    // TODO: to make more robust track the `status` of the client.
    //
    process(clientSocket) {
        console.log('processing packet');
	      switch (this.type) {
	      case PacketType.Handshake:
            console.log('Sending handshake response... ', handshakeResponseData[0],
                        ' len: ', handshakeResponseData.length);
            clientSocket.write(handshakeResponseData);
            break;
		        // a.SetStatus(constants.StatusHandshake)
		        // logger.Log.Debugf("Session handshake Id=%d, Remote=%s", a.Session.ID(), a.RemoteAddr())

	      case PacketType.HandshakeAck:
            console.log('RECEIVED HANDSHAKE ACK');

            if (clientDisconnected && !heartbeatInterval) {
                heartbeatInterval = setInterval(() => {
                    if (clientSocket && !clientSocket.destroyed) {
                        sendHeartbeat(clientSocket);
                    }
                }, 2000);
            }
            break;

	      case PacketType.Data:
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

	      case PacketType.Heartbeat:
		        // expected
            break;
	      }

        // TODO: Return an error here
	      // a.SetLastAt()
	      // return nil
    }
}

class RawPackets {
    constructor(buffer) {
        this.buffer = buffer;
    }

    length() { return this.buffer.length; }

    // Advance buffer by `n` bytes. Returns the advanced values as a buffer slice.
    advance(nbytes) {
        if (this.length() < nbytes) {
            throw new Error(`could not advance by ${nbytes} bytes`);
        } else {
            const slice = this.buffer.slice(0, nbytes);
            this.buffer = this.buffer.slice(nbytes);
            return slice;
        }
    }

    headerForward() {
        const header = this.advance(HEADER_LENGTH);
        const type = header[0];

	      if (type < PacketType.Handshake || type > PacketType.Kick) {
            throw new Error('Wrong packet type');
	      }

        const size = header.readIntBE(1, HEADER_LENGTH-1);

	      // packet length limitation
	      if (size > MAX_PACKET_SIZE) {
		        throw new Error('Packet size too large');
	      }

	      return [size, type];
    }

    decode() {
        let packets = [];

	      // check length
	      if (this.buffer.length < HEADER_LENGTH) {
		        throw new Error('Length smaller than a header.');
	      }

	      // first time
	      let [size, type] = this.headerForward();

	      while (size <= this.length()) {
		        const p = new Packet(type, size, this.advance(size));
		        packets.push(p);

		        if (this.length() < HEADER_LENGTH) {
			          break;
		        }

		        [size, type] = this.headerForward();
	      }

	      return packets;
    }

}

const server = net.createServer((socket) => {
    console.log('======= New Connection ========');

    clientSocket = socket;

    socket.on('data', (buffer) => {
        console.log(`Received ${buffer.length} bytes of data`);
        console.log('Decoding packets...');
        const rawPackets = new RawPackets(buffer);
        const packets = rawPackets.decode();

        console.log(`Decoded ${packets.length} packet(s)`);

        for (let p of packets) {
            p.process(socket);
        }
    });

    socket.on('end', () => {
        clientDisconnected = true;
        console.log('Client disconnected :(');
    });
});

console.log(`Listening on ${HOST}:${PORT}`);
server.listen(PORT, HOST);
