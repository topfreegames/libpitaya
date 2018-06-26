const zlib = require('zlib');
const pkt = require('./packet.js');

const Type = Object.freeze({
    Request : 0x00,
    Notify  : 0x01,
    Response: 0x02,
    Push    : 0x03,
});

const Mask = Object.freeze({
    Error           : 0x20,
    Gzip            : 0x10,
    MsgRouteCompress: 0x01,
    MsgTypeMask     : 0x07,
    MsgRouteLength  : 0xFF,
});

const MSG_HEAD_LENGTH = 0x02;

const Errors = Object.freeze({
	  WrongMessageType: new Error("wrong message type"),
	  InvalidMessage: new Error("invalid message"),
	  RouteInfoNotFound: new Error("route info not found in dictionary"),
});

function createEmptyMessage() {
    return {
        type: -1,
        id: -1,
        route: '',
        data: null,
        gzipped: false,
        routeCompressed: false,
        isError: false,
    };
}

function createResponseMessage(id, data) {
    return {
        id,
        data,
        type: Type.Response,
        routeCompressed: false,
        isError: false,
    };
}

let codeToRoute = {};
let routeToCode = {};

function invalidType(t) {
	  return t < Type.Request || t > Type.Push;
}

function routable(t) {
	  return t === Type.Request || t === Type.Notify || t === Type.Push;
}

function encode(msg) {
	  if (invalidType(msg.type)) {
		    return [null, Errors.WrongMessageType];
	  }

	  let flag = msg.type << 1;

	  const code = routeToCode[msg.route];
	  if (code) {
		    flag |= Mask.MsgRouteCompress;
	  }

	  if (msg.isError) {
		    flag |= Mask.Error;
	  }

	  const flagBuf = Buffer.alloc(1);
    flagBuf.writeUInt8(flag, 0);

    let msgIdBuf = null;
	  if (msg.type === Type.Request || msg.type === Type.Response) {
		    let n = msg.id;
		    // variant length encode
        const buf = [];
		    while (true) {
			      const b = (n % 128);
			      n >>= 7;
			      if (n !== 0) {
				        buf.push(b+128);
			      } else {
				        buf.push(b);
				        break;
			      }
		    }
        msgIdBuf = Buffer.alloc(buf.length);
        for (let i = 0; i < msgIdBuf.length; i++) {
            msgIdBuf.writeUInt8(buf[i], i);
        }
	  }

    let routeBuf = null;
	  if (routable(msg.type)) {
        let buf = [];
		    if (code) {
			      buf.push((code>>8)&0xFF);
			      buf.push(code&0xFF);
		    } else {
			      buf.push(msg.route.length);
            for (let i = 0; i < msg.route.length; i++) {
			          buf.push(msg.route[i]);
            }
		    }
        routeBuf = Buffer.alloc(buf.length);
        for (let i = 0; i < msgIdBuf.length; i++) {
            routeBuf.writeUInt8(buf[i], i);
        }
	  }

    const dataCompression = false;
	  if (dataCompression) {
        const compressedData = zlib.deflateSync(msg.data);

		    if (compressedData.length < msg.data) {
			      msg.data = compressedData;
			      flagBuf[0] |= Mask.Gzip;
		    }
	  }

    const dataBuf = Buffer.alloc(msg.data.length);
    dataBuf.write(msg.data.toString(), 0);

    const finalBuf = Buffer.concat([
        flagBuf,
        msgIdBuf ? msgIdBuf : Buffer.alloc(0),
        routeBuf ? routeBuf : Buffer.alloc(0),
        dataBuf  ? dataBuf  : Buffer.alloc(0),
    ]);

	  return [finalBuf, null];
}


// See ref: https://github.com/topfreegames/pitaya/blob/master/docs/communication_protocol.md
function decode(buf) {
    if (buf.length < MSG_HEAD_LENGTH) {
        console.log('ERROR: buffer smaller than the message header');
        return [null, Errors.InvalidMessage];
    }
    let msg = createEmptyMessage();
    const flag = buf[0];
    let offset = 1;
    msg.type = (flag >> 1) & Mask.MsgType;

    if (invalidType(msg.type)) {
        return [null, Errors.WrongMessageType];
    }

    if (msg.type == Type.Request || msg.type == Type.Response) {
        let id = 0;
        // little end byte order
        // WARNING: must can be stored in 64 bits integer
        // variant length encode
        for (let i = offset; i < buf.length; i++) {
            const b = buf[i];
            id += (b&0x7F) << (7*(i-offset));
            if (b < 128) {
                offset = i + 1;
                break;
            }
        }
        msg.id = id;
    }

    msg.isError = Boolean(flag&Mask.Error == Mask.Error);

    if (routable(msg.type)) {
        if (flag&Mask.MsgRouteCompress === 1) {
            msg.routeCompressed = true;
            const code = buf.readUInt16BE(offset);
            console.log(`CODE IS ${code}`);
            const route = codeToRoute[code];
            if (!route) {
                return [null, Error.RouteInfoNotFound];
            }
            msg.route = route;
            offset += 2;
        } else {
            msg.routeCompressed = false;
            const rl = buf[offset];
            offset++;
            msg.route = buf.slice(offset, offset + rl).toString();
            offset += rl;
        }
    }

    msg.data = buf.slice(offset);

    if ((flag&Mask.Gzip) === Mask.Gzip) {
        msg.gzipped = true;
        msg.data = zlib.inflateSync(msg.data);
    }

    msg.data = msg.data.toString();

    return [msg, null];
}

exports.encode = encode;
exports.decode = decode;
exports.createResponseMessage = createResponseMessage;
