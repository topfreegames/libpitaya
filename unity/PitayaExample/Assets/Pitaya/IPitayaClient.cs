using System;
using System.Collections.Generic;
using System.IO;
using Google.Protobuf;
using UnityEngine;

namespace Pitaya
{
    public interface IPitayaClient
    {
        event Action<PitayaNetWorkState, NetworkError> NetWorkStateChangedEvent;
        
        int Quality { get; }
        PitayaClientState State { get; }
        ISerializerFactory SerializerFactory { get; set; }
        void Connect(string host, int port, string handshakeOpts = null);
        void Connect(string host, int port, Dictionary<string, string> handshakeOpts);

        /// <summary cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)">
        /// </summary>
        void Request<TResponse>(string route, object msg, Action<TResponse> action, Action<PitayaError> errorAction, int timeout = -1);
        
        /// <summary cref="Notify(string, object, int)">
        /// </summary>
        void Notify(string route, object msg, int timeout = -1);
        
        /// <summary cref="OnRoute&lt;T&gt;(string, Action&lt;T&gt;)">
        /// </summary>
        void OnRoute<T>(string route, Action<T> action);
        
        void OffRoute(string route);
        void Disconnect();
        void Dispose();
        void ClearAllCallbacks();
        void RemoveAllOnRouteEvents();
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        void Request(string route, Action<string> action, Action<PitayaError> errorAction);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        void Request<T>(string route, Action<T> action, Action<PitayaError> errorAction);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        void Request(string route, string msg, Action<string> action, Action<PitayaError> errorAction);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        void Request<T>(string route, IMessage msg, Action<T> action, Action<PitayaError> errorAction);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        void Request<T>(string route, IMessage msg, int timeout, Action<T> action, Action<PitayaError> errorAction);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Request&lt;TResponse&gt;(string, object, Action&lt;TResponse&gt;, Action&lt;PitayaError&gt;, int)"/> instead.</para>
        /// </summary>
        void Request(string route, string msg, int timeout, Action<string> action, Action<PitayaError> errorAction);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        void Notify(string route, IMessage msg);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        void Notify(string route, int timeout, IMessage msg);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        void Notify(string route, string msg);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="Notify(string, object, int)"/> instead.</para>
        /// </summary>
        void Notify(string route, int timeout, string msg);
        
        /// <summary>
        /// <para>DEPRECATED. Use <see cref="OnRoute(string, Action&lt;T&gt;)"/> instead.</para>
        /// </summary>
        void OnRoute(string route, Action<string> action);
    }
}