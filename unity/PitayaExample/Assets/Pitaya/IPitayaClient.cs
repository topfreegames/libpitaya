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
        void Connect(string host, int port, string handshakeOpts = null);
        void Connect(string host, int port, Dictionary<string, string> handshakeOpts);
        void Request(string route, Action<string> action, Action<PitayaError> errorAction);
        void Request<T>(string route, Action<T> action, Action<PitayaError> errorAction);
        void Request(string route, string msg, Action<string> action, Action<PitayaError> errorAction);
        void Request<T>(string route, IMessage msg, Action<T> action, Action<PitayaError> errorAction);
        void Request<T>(string route, IMessage msg, int timeout, Action<T> action, Action<PitayaError> errorAction);
        void Request(string route, string msg, int timeout, Action<string> action, Action<PitayaError> errorAction);
        void Notify(string route, IMessage msg);
        void Notify(string route, int timeout, IMessage msg);
        void Notify(string route, string msg);
        void Notify(string route, int timeout, string msg);
        void OnRoute(string route, Action<string> action);
        void OnRoute<T>(string route, Action<T> action);
        void OffRoute(string route);
        void Disconnect();
        void Dispose();
        void ClearAllCallbacks();
        void RemoveAllOnRouteEvents();
    }
}