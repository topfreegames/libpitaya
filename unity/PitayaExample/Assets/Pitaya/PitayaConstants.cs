using System.Collections.Generic;

namespace Pitaya
{
    public class PitayaError
    {
        public readonly string Code;
        public readonly string Msg;
        public readonly IDictionary<string, string> Metadata;

        public PitayaError(string code, string msg, IDictionary<string, string> metadata = null)
        {
            Code = code;
            Msg = msg;
            Metadata = metadata;
        }
    }

    public class NetworkError
    {
        public readonly string Error;
        public readonly string Description;

        public NetworkError(string error, string description)
        {
            Error = error;
            Description = description;
        }
    }

    public enum PitayaNetWorkState
    {
        Closed,
        Connecting,
        FailToConnect,
        Connected,
        Disconnected,
        Timeout,
        Error,
        Kicked
    }

    public enum PitayaLogLevel
    {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Disable = 4
    }

    public enum PitayaClientState
    {
        Inited = 0,//Waiting for the player to start a connection
        Connecting = 1,
        Connected = 2,
        Disconnecting = 3,
        Unknown = 4 // you should discart the client and create a new instance
    }

    public static class PitayaConstants
    {
        public const int PcRcOk = 0;
        public const int PcRcError = -1;
        public const int PcRcTimeout = -2;
        public const int PcRcInvalidJson = -3;
        public const int PcRcInvalidArg = -4;
        public const int PcRcNoTrans = -5;
        public const int PcRcInvalidThread = -6;
        public const int PcRcTransError = -7;
        public const int PcRcInvalidRoute = -8;
        public const int PcRcInvalidState = -9;
        public const int PcRcNotFound = -10;
        public const int PcRcReset = -11;
        public const int PcRcServerError = -12;
        public const int PcRcUvError = -13;
        public const int PcRcNoSuchFile = -14;
        public const int PcRcMin = -15;

        public const int PcStNotInited = 0;
        public const int PcStInited = 1;
        public const int PcStConnecting = 2;
        public const int PcStConnected = 3;
        public const int PcStDisconnecting = 4;
        public const int PcStUnknown = 5;

        public const int PcWithoutTimeout = -1;

        public const int PcEvUserDefinedPush = 0;
        public const int PcEvConnected = 1;
        public const int PcEvConnectError = 2;
        public const int PcEvConnectFailed = 3;
        public const int PcEvDisconnect = 4;
        public const int PcEvKickedByServer = 5;
        public const int PcEvUnexpectedDisconnect = 6;
        public const int PcEvProtoError = 7;
        public const int PcEvReconnectFailed = 8;
        public const int PcEvReconnectStarted = 9;
        public const int PcEvCount = 10;

        public const int PcEvInvalidHandlerId = -1;
        
        public const string SerializerProtobuf = "protobuf";
        public const string SerializerJson = "json";

        public const string PitayaInternalError = "PIT-LIB-INT";
    }

}
