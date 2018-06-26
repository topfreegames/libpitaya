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

let handshakeResponseData;
let heartbeatResponseData;

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
        return Buffer.concat([headerBuf, data]);
    } else {
        return headerBuf;
    }
};

function encodeHanshakeAndHeartbeatResponse(heartbeatInterval) {
    // Hardcoded handshake data
    const hData = {
        'code': 200,
        'sys': {
            'heartbeat': heartbeatInterval,
            'dict': {
                'connector.getsessiondata': 1,
                'connector.setsessiondata': 2,
                'room.room.getsessiondata': 3,
                'onMessage':                4,
                'onMembers':                5,
            },
            'serializer': 'json',
        }
    };
    const data = JSON.stringify(hData);
    console.log(data);

    handshakeResponseData = encode(PacketType.Handshake, Buffer.from(data));
    heartbeatResponseData = encode(PacketType.Heartbeat);

    console.log('Handshake response data: ', handshakeResponseData);
}

class Packet {
    constructor(type, length, data) {
        this.type = type;
        this.length = length;
        this.data = data;
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

function sendHandshakeResponse(socket) {
    socket.write(handshakeResponseData);
}

function sendHeartbeat(socket) {
    console.log('sending heartbeat');
    socket.write(heartbeatResponseData);
}

exports.RawPackets = RawPackets;
exports.HEADER_LENGTH = HEADER_LENGTH;
exports.MAX_PACKET_SIZE = MAX_PACKET_SIZE;
exports.PacketType = PacketType;
exports.encode = encode;
exports.Packet = Packet;
exports.sendHandshakeResponse = sendHandshakeResponse;
exports.sendHeartbeat = sendHeartbeat;
exports.encodeHanshakeAndHeartbeatResponse = encodeHanshakeAndHeartbeatResponse;
