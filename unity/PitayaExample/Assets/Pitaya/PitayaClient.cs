using System;
using System.Collections.Generic;
using System.IO;
using Google.Protobuf;
using Newtonsoft.Json;
using UnityEngine;

namespace Pitaya
{
    public class PitayaClient : IDisposable, IPitayaClient, IPitayaListener
    {
        public event Action<PitayaNetWorkState, NetworkError> NetWorkStateChangedEvent;

        private const int DEFAULT_CONNECTION_TIMEOUT = 30;

        private IntPtr _client = IntPtr.Zero;
        private EventManager _eventManager;
        private bool _disposed;
        private uint _reqUid;
        private Dictionary<uint, Action<string, string>> _requestHandlers;

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
            Init(certificateName, certificateName != null, false, enableReconnect, connectionTimeout);
        }

        ~PitayaClient()
        {
            Dispose();
        }

        private void Init(string certificateName, bool enableTlS, bool enablePolling, bool enableReconnect, int connTimeout)
        {
            _eventManager = new EventManager();
            _client = PitayaBinding.CreateClient(enableTlS, enablePolling, enableReconnect, connTimeout, this);

            if (certificateName != null)
            {
#if UNITY_EDITOR
                if (File.Exists(certificateName))
                    PitayaBinding.SetCertificatePath(certificateName);
                else
                    PitayaBinding.SetCertificateName(certificateName);
#else
                PitayaBinding.SetCertificateName(certificateName);
#endif
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

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        public void Request(string route, Action<string> action, Action<PitayaError> errorAction)
        {
            Request(route, (string) null, action, errorAction);
        }
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        public void Request<T>(string route, Action<T> action, Action<PitayaError> errorAction)
        {
            Request(route, null, action, errorAction);
        }
        
        /// <summary cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)">
        /// </summary>
        public void Request<TResponse>(string route, object msg, Action<TResponse> action, Action<PitayaError> errorAction, int timeout = -1)
        {
            IPitayaSerializer serializer = new JsonSerializer();
            if ((IMessage)msg != null) serializer = new ProtobufSerializer(PitayaBinding.ClientSerializer(_client));
            RequestInternal(route, msg, timeout, serializer, action, errorAction);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        public void Request(string route, string msg, Action<string> action, Action<PitayaError> errorAction)
        {
            Request(route, msg, -1, action, errorAction);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        public void Request<T>(string route, IMessage msg, Action<T> action, Action<PitayaError> errorAction)
        {
            Request(route, msg, -1, action, errorAction);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        public void Request<T>(string route, IMessage msg, int timeout, Action<T> action, Action<PitayaError> errorAction)
        {
            ProtobufSerializer.SerializationFormat format = PitayaBinding.ClientSerializer(_client);
            RequestInternal(route, msg, timeout, new ProtobufSerializer(format), action, errorAction);
        }
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        public void Request(string route, string msg, int timeout, Action<string> action, Action<PitayaError> errorAction)
        {
            RequestInternal(route, msg, timeout, new LegacyJsonSerializer(), action, errorAction);
        }
        
        void RequestInternal<TResponse, TRequest>(string route, TRequest msg, int timeout, IPitayaSerializer serializer, Action<TResponse> action, Action<PitayaError> errorAction)
        {
            _reqUid++;
            
            Action<byte[]> responseAction = res => { action(serializer.Decode<TResponse>(res)); };

            _eventManager.AddCallBack(_reqUid, responseAction, errorAction);

            PitayaBinding.Request(_client, route, serializer.Encode(msg), _reqUid, timeout);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        public void Notify(string route, IMessage msg)
        {
            Notify(route, -1, msg);
        }
        
        public void Notify(string route, object msg, int timeout = -1)
        {
            IPitayaSerializer serializer = new JsonSerializer();
            if ((IMessage)msg != null) serializer = new ProtobufSerializer(PitayaBinding.ClientSerializer(_client));
            NotifyInternal(route, msg, serializer, timeout);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        public void Notify(string route, int timeout, IMessage msg)
        {
            ProtobufSerializer.SerializationFormat format = PitayaBinding.ClientSerializer(_client);
            NotifyInternal(route, msg, new ProtobufSerializer(format), timeout);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        public void Notify(string route, string msg)
        {
            Notify(route, -1, msg);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        public void Notify(string route, int timeout, string msg)
        {
            byte[] bytes = new LegacyJsonSerializer().Encode(msg);
            PitayaBinding.Notify(_client, route, bytes, timeout);
        }
        
        private void NotifyInternal(string route, object msg, IPitayaSerializer serializer, int timeout = -1)
        {
            PitayaBinding.Notify(_client, route, serializer.Encode(msg), timeout);
        }

        /// <summary>
        /// <para>DEPRECATED. Use <see cref="OnRoute(string, Action&lt;T&gt;)"/> instead.</para>
        /// </summary>
        public void OnRoute(string route, Action<string> action)
        {
            OnRouteInternal(route, action, new LegacyJsonSerializer());
        }

        // start listening to a route
        public void OnRoute<T>(string route, Action<T> action)
        {
            IPitayaSerializer serializer = new JsonSerializer();
            if (typeof(IMessage).IsAssignableFrom(typeof(T))) serializer = new ProtobufSerializer(PitayaBinding.ClientSerializer(_client));

            OnRouteInternal(route, action, serializer);
        }
        
        private void OnRouteInternal<T>(string route, Action<T> action, IPitayaSerializer serializer)
        {
            Action<byte[]> responseAction = res => { action(serializer.Decode<T>(res)); };
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
            _eventManager.InvokeCallBack(rid, data);
        }

        public void OnRequestError(uint rid, PitayaError error)
        {
            _eventManager.InvokeErrorCallBack(rid, error);
        }

        public void OnNetworkEvent(PitayaNetWorkState state, NetworkError error)
        {
            if (NetWorkStateChangedEvent != null) NetWorkStateChangedEvent.Invoke(state, error);
        }

        public void OnUserDefinedPush(string route, byte[] serializedBody)
        {
            _eventManager.InvokeOnEvent(route, serializedBody);
        }

        public void Dispose()
        {
            Debug.Log(string.Format("PitayaClient Disposed {0}", _client));
            if (_disposed)
                return;

            if (_eventManager != null) _eventManager.Dispose();

            _reqUid = 0;
            PitayaBinding.Disconnect(_client);
            PitayaBinding.Dispose(_client);

            _client = IntPtr.Zero;
            _disposed = true;
        }

        public void ClearAllCallbacks()
        {
            _eventManager.ClearAllCallbacks();
        }

        public void RemoveAllOnRouteEvents()
        {
            _eventManager.RemoveAllOnRouteEvents();
        }
    }
}
