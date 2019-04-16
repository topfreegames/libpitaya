using System;
using System.Collections.Generic;
using Google.Protobuf;
using UnityEngine;

namespace Pitaya
{
    public class PitayaClient : IDisposable, IPitayaListener
    {
        public event Action<PitayaNetWorkState> NetWorkStateChangedEvent;

        private const int DEFAULT_CONNECTION_TIMEOUT = 30;

        private IntPtr _client = IntPtr.Zero;
        private EventManager _eventManager;
        private bool _disposed;
        private uint _reqUid;
        private Dictionary<uint, Action<string, string>> _requestHandlers;
        private TypeSubscriber<uint> _typeRequestSubscriber;
        private TypeSubscriber<string> _typePushSubscriber;

        public PitayaClient()
        {
            Init(null, false, false, false, DEFAULT_CONNECTION_TIMEOUT);
        }

        public PitayaClient(int connectionTimeout)
        {
            Init(null, false, false, false, connectionTimeout);
        }

        public PitayaClient(string certificateName = null)
        {
            Init(certificateName, certificateName != null, false, false, DEFAULT_CONNECTION_TIMEOUT);
        }

        public PitayaClient(bool enableReconnect = false, string certificateName = null, int connectionTimeout = DEFAULT_CONNECTION_TIMEOUT)
        {
            Init(certificateName, certificateName != null, false, enableReconnect, DEFAULT_CONNECTION_TIMEOUT);
        }

        ~PitayaClient()
        {
            Dispose();
        }

        private void Init(string certificateName, bool enableTlS, bool enablePolling, bool enableReconnect, int connTimeout)
        {
            _eventManager = new EventManager();
            _typeRequestSubscriber = new TypeSubscriber<uint>();
            _typePushSubscriber = new TypeSubscriber<string>();
            _client = PitayaBinding.CreateClient(enableTlS, enablePolling, enableReconnect, connTimeout, this);

            if (certificateName != null)
            {
                PitayaBinding.SetCertificateName(certificateName);
            }
        }

        public static void SetLogLevel(PitayaLogLevel level)
        {
            PitayaBinding.SetLogLevel(level);
        }

        public int Quality
        {
            get { return PitayaBinding.Quality(_client); }
        }


        public PitayaClientState State
        {
            get { return PitayaBinding.State(_client); }

        }

        public void Connect(string host, int port, string handshakeOpts = null)
        {
            PitayaBinding.Connect(_client, host, port, handshakeOpts);
        }

        public void Connect(string host, int port, Dictionary<string, string> handshakeOpts)
        {
            var opts = Pitaya.SimpleJson.SimpleJson.SerializeObject(handshakeOpts);
            PitayaBinding.Connect(_client, host, port, opts);
        }

        public void Request(string route, Action<string> action, Action<PitayaError> errorAction)
        {
            Request(route, (string)null, action, errorAction);
        }

        public void Request<T>(string route, Action<T> action, Action<PitayaError> errorAction)
        {
            Request(route, null, action, errorAction);
        }

        public void Request(string route, string msg, Action<string> action, Action<PitayaError> errorAction)
        {
            Request(route, msg, -1, action, errorAction);
        }

        public void Request<T>(string route, IMessage msg, Action<T> action, Action<PitayaError> errorAction)
        {
            Request(route, msg, -1, action, errorAction);
        }

        public void Request<T>(string route, IMessage msg, int timeout, Action<T> action, Action<PitayaError> errorAction)
        {
            _reqUid++;
            _typeRequestSubscriber.Subscribe(_reqUid, typeof(T));

            Action<object> responseAction = res => { action((T) res); };

            _eventManager.AddCallBack(_reqUid, responseAction, errorAction);

            var serializer = PitayaBinding.ClientSerializer(_client);

            PitayaBinding.Request(_client, route, ProtobufSerializer.Encode(msg, serializer), _reqUid, timeout);
        }

        public void Request(string route, string msg, int timeout, Action<string> action, Action<PitayaError> errorAction)
        {
            _reqUid++;
            Action<object> responseAction = res => { action((string) res); };

            _eventManager.AddCallBack(_reqUid, responseAction, errorAction);

            PitayaBinding.Request(_client, route,JsonSerializer.Encode(msg), _reqUid, timeout);
        }

        public void Notify(string route, IMessage msg)
        {
            Notify(route, -1, msg);
        }

        public void Notify(string route, int timeout, IMessage msg)
        {
            var serializer = PitayaBinding.ClientSerializer(_client);
            PitayaBinding.Notify(_client, route, ProtobufSerializer.Encode(msg,serializer), timeout);
        }

        public void Notify(string route, string msg)
        {
            Notify(route, -1, msg);
        }

        public void Notify(string route, int timeout, string msg)
        {
            PitayaBinding.Notify(_client, route, JsonSerializer.Encode(msg), timeout);
        }

        public void OnRoute(string route, Action<string> action)
        {
            Action<object> responseAction = res => { action((string) res); };
            _eventManager.AddOnRouteEvent(route, responseAction);
        }

        // start listening to a route
        public void OnRoute<T>(string route, Action<T> action)
        {
            _typePushSubscriber.Subscribe(route, typeof(T));
            Action<object> responseAction = res => { action((T) res); };

            _eventManager.AddOnRouteEvent(route, responseAction);
        }

        public void OffRoute(string route)
        {
            _eventManager.RemoveOnRouteEvent(route);
        }

        public void Disconnect()
        {
            PitayaBinding.Disconnect(_client);
        }

        //---------------Pitaya Listener------------------------//

        public void OnRequestResponse(uint rid, byte[] data)
        {
            object decoded;
            if (_typeRequestSubscriber.HasType(rid))
            {
                var type = _typeRequestSubscriber.GetType(rid);
                decoded = ProtobufSerializer.Decode(data, type, PitayaBinding.ClientSerializer(_client));
            }
            else
            {
                decoded = JsonSerializer.Decode(data);
            }

            _eventManager.InvokeCallBack(rid, decoded);
        }

        public void OnRequestError(uint rid, PitayaError error)
        {
            _eventManager.InvokeErrorCallBack(rid, error);
        }

        public void OnNetworkEvent(PitayaNetWorkState state)
        {
            if(NetWorkStateChangedEvent != null ) NetWorkStateChangedEvent.Invoke(state);
        }

        public void OnUserDefinedPush(string route, byte[] serializedBody)
        {
            object decoded;
            if (_typePushSubscriber.HasType(route))
            {
                var type = _typePushSubscriber.GetType(route);
                decoded = ProtobufSerializer.Decode(serializedBody, type, PitayaBinding.ClientSerializer(_client));
            }
            else
            {
                decoded = JsonSerializer.Decode(serializedBody);
            }

           _eventManager.InvokeOnEvent(route, decoded);
        }

        public void Dispose()
        {
            Debug.Log(string.Format("PitayaClient Disposed {0}", _client));
            if (_disposed)
                return;

            if(_eventManager != null ) _eventManager.Dispose();

            _reqUid = 0;
            PitayaBinding.Disconnect(_client);
            PitayaBinding.Dispose(_client);

            _client = IntPtr.Zero;
            _disposed = true;
        }
    }
}
