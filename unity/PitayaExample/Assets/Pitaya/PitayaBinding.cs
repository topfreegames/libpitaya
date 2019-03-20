using System;
using System.IO;
using System.Text;
using UnityEngine;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using AOT;
using Pitaya;
using Pitaya.SimpleJson;
using Protos;

//typedef void (*request_callback)(pc_client_t* client, unsigned int cbid, const char* resp);
using NativeRequestCallback = System.Action<System.IntPtr, uint, System.IntPtr>;
// typedef void (*request_error_callback)(pc_client_t* client, unsigned int cbid, pc_error_t* error);
using NativeErrorCallback = System.Action<System.IntPtr, uint, System.IntPtr>;

using NativeNotifyCallback = System.Action<System.IntPtr, System.IntPtr>;


internal delegate void NativeEventCallback(IntPtr client, int ev, IntPtr exData, IntPtr arg1Ptr, IntPtr arg2Ptr);

internal delegate void NativePushCallback(IntPtr client, IntPtr route, IntPtr payloadBuffer);

internal delegate void NativeAssertCallback(IntPtr e, IntPtr file, int line);

public delegate void NativeLogFunction(PitayaLogLevel level, string msg);

#pragma warning disable 649
internal struct PitayaBindingError
{
    public int Code;
    public PitayaBuffer Buffer;
    public int Uvcode;
}

internal struct PitayaBuffer
{
    public IntPtr Data;
    public long Len;
}
#pragma warning restore 649

namespace Pitaya
{
    public interface IPitayaListener
    {
        void OnRequestResponse(uint rid, byte[] body);
        void OnRequestError(uint rid, PitayaError error);
        void OnNetworkEvent(PitayaNetWorkState state);
        void OnUserDefinedPush(string route, byte[] serializedBody);
    }

    public static class PitayaBinding
    {
        private static readonly NativeNotifyCallback NativeNotifyCallback;
        private static readonly NativeRequestCallback NativeRequestCallback;
        private static readonly NativeEventCallback NativeEventCallback;
        private static readonly NativePushCallback NativePushCallback;
        private static readonly NativeErrorCallback NativeErrorCallback;

        private static readonly Dictionary<IntPtr, WeakReference> Listeners = new Dictionary<IntPtr, WeakReference>();
        private static readonly Dictionary<IntPtr, int> EventHandlersIds = new Dictionary<IntPtr, int>();
        private static PitayaLogLevel _currentLogLevel = PitayaLogLevel.Disable;

        private static void DLog(object data)
        {
            if (_currentLogLevel != PitayaLogLevel.Disable)
            {
                Debug.Log(data);
            }
        }

        static PitayaBinding()
        {
            NativeRequestCallback = OnRequest;
            NativeEventCallback = OnEvent;
            NativePushCallback = OnPush;
            NativeNotifyCallback = OnNotify;
            NativeErrorCallback = OnError;

            string platform;
            switch (Application.platform)
            {
                case RuntimePlatform.Android:
                    platform = "android";
                    break;
                case RuntimePlatform.LinuxEditor:
                case RuntimePlatform.LinuxPlayer:
                    platform = "linux";
                    break;
                case RuntimePlatform.WindowsEditor:
                case RuntimePlatform.WindowsPlayer:
                    platform = "windows";
                    break;
                case RuntimePlatform.IPhonePlayer:
                    platform = "ios";
                    break;
                case RuntimePlatform.OSXEditor:
                case RuntimePlatform.OSXPlayer:
                    platform = "mac";
                    break;
                default:
                    platform = Application.platform.ToString();
                    break;
            }

            SetLogFunction(LogFunction);
            NativeLibInit((int)_currentLogLevel, null, null, OnAssert, platform, BuildNumber(), Application.version);
        }

        private static void SetLogFunction(NativeLogFunction fn)
        {
            int rc = NativeInitLogFunction(fn);
            if (rc != 0)
            {
                throw new Exception("Cannot initialize log function");
            }
        }

        private static string BuildNumber()
        {
            switch (Application.platform)
            {
                case RuntimePlatform.IPhonePlayer:
                    return _PitayaGetCFBundleVersion();
                case RuntimePlatform.Android:
                    return _AndroidBuildNumber();
                default:
                    return "1";
            }
        }

        private static string _AndroidBuildNumber()
        {
            var contextCls = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
            var context = contextCls.GetStatic<AndroidJavaObject>("currentActivity");
            var packageMngr = context.Call<AndroidJavaObject>("getPackageManager");
            var packageName = context.Call<string>("getPackageName");
            var packageInfo = packageMngr.Call<AndroidJavaObject>("getPackageInfo", packageName, 0);
            return (packageInfo.Get<int>("versionCode")).ToString();
        }

        public static IntPtr CreateClient(bool enableTls, bool enablePolling, bool enableReconnect, int connTimeout, IPitayaListener listener)
        {
            var client = NativeCreate(enableTls, enablePolling, enableReconnect, connTimeout);
            if (client == IntPtr.Zero)
            {
                throw new Exception("Fail to create a client");
            }

            var handlerId = NativeAddEventHandler(client, NativeEventCallback, IntPtr.Zero, IntPtr.Zero);
            NativeAddPushHandler(client, NativePushCallback);
            Listeners[client] = new WeakReference(listener);
            EventHandlersIds[client] = handlerId;

            return client;
        }

        public static void SetLogLevel(PitayaLogLevel logLevel)
        {
            NativeLibSetLogLevel((int)logLevel);
            _currentLogLevel = logLevel;
        }

        public static void Connect(IntPtr client, string host, int port)
        {
            CheckClient(client);
            NativeConnect(client, host, port, null);
        }

        public static void Disconnect(IntPtr client)
        {
            CheckClient(client);
            NativeDisconnect(client);
        }

        public static void SetCertificateName(string name)
        {
            var certPath = FindCertPathFromName(name);
            NativeSetCertificatePath(certPath, null);
        }

        public static void Request(IntPtr client, string route, byte[] msg, uint reqtId, int timeout)
        {
            var length = 0;
            if (msg != null)
                length = msg.Length;

            var rc = NativeBinaryRequest(client, route, msg, length, reqtId, timeout, NativeRequestCallback, NativeErrorCallback);

            if (rc != PitayaConstants.PcRcOk)
            {
                DLog(string.Format("request - failed to perform request {0}", RcToStr(rc)));

                WeakReference reference;
                if (!Listeners.TryGetValue(client, out reference) || !reference.IsAlive) return;
                var listener = reference.Target as IPitayaListener;
                if (listener != null) listener.OnRequestError(reqtId, new PitayaError(PitayaConstants.PitayaInternalError, "Failed to send request"));
            }
        }

        public static void Notify(IntPtr client, string route, byte[] msg, int timeout)
        {
            var length = 0;
            if (msg != null)
                length = msg.Length;

            NativeBinaryNotify(client, route, msg, length, IntPtr.Zero, timeout, NativeNotifyCallback);
        }

        public static int Quality(IntPtr client)
        {
            CheckClient(client);
            return NativeQuality(client);
        }

        public static PitayaClientState State(IntPtr client)
        {
            CheckClient(client);
            return (PitayaClientState)NativeState(client);
        }

        public static void Dispose(IntPtr client)
        {
            NativeRemoveEventHandler(client, EventHandlersIds[client]);

            Listeners.Remove(client);
            EventHandlersIds.Remove(client);

            NativeDestroy(client);
        }

        public static PitayaSerializer ClientSerializer(IntPtr client)
        {
            IntPtr nativeSerializer = NativeSerializer(client);
            var serializer = Marshal.PtrToStringAnsi(nativeSerializer);
            NativeFreeSerializer(nativeSerializer);
            return PitayaConstants.SerializerJson.Equals(serializer) ? PitayaSerializer.Json : PitayaSerializer.Protobuf;
        }

        public static void AddPinnedPublicKeyFromCertificateString(string caString)
        {
            var rc = NativeAddPinnedPublicKeyFromCertificateString(caString);

            if (rc != PitayaConstants.PcRcOk)
            {
                throw new Exception(string.Format("AddPineedPublicKeyFromCertificateString: {0}", RcToStr(rc)));
            }

            SkipKeyPinCheck(false);
        }

        public static void AddPinnedPublicKeyFromCertificateFile(string name)
        {
            var certPath = FindCertPathFromName(name);
            int rc = NativeAddPinnedPublicKeyFromCertificateFile(certPath);

            if (rc != PitayaConstants.PcRcOk)
            {
                throw new Exception(string.Format("AddPineedPublicKeyFromCertificateFile: {0}", RcToStr(rc)));
            }

            SkipKeyPinCheck(false);
        }

        public static void SkipKeyPinCheck(bool shouldSkip)
        {
            NativeSkipKeyPinCheck(shouldSkip);
        }

        public static void ClearPinnedPublicKeys()
        {
            NativeClearPinnedPublicKeys();
        }

        //--------------------HELPER METHODS----------------------------------//
        private static string EvToStr(int ev)
        {
            return Marshal.PtrToStringAnsi(NativeEvToStr(ev));
        }

        private static string RcToStr(int rc)
        {
            return Marshal.PtrToStringAnsi(NativeRcToStr(rc));
        }

        private static string FindCertPathFromName(string name)
        {
            string certPath = Path.Combine(Application.streamingAssetsPath, name);

            if (Application.platform == RuntimePlatform.Android)
            {
                var persistentPath = Path.Combine(Application.persistentDataPath, name);

                if (!PlayerPrefs.HasKey(persistentPath))
                {
                    var reader = new WWW(certPath);
                    while (!reader.isDone) { }
                    System.IO.File.WriteAllBytes(persistentPath, reader.bytes);
                    PlayerPrefs.SetInt(persistentPath, 1);
                }

                certPath = persistentPath;
            }

            return certPath;
        }

        private static PitayaError CreatePitayaError(PitayaBindingError errorBinding, PitayaSerializer serializer)
        {
            var rawData = new byte[errorBinding.Buffer.Len];
            Marshal.Copy(errorBinding.Buffer.Data, rawData, 0, (int)errorBinding.Buffer.Len);

            if (serializer == PitayaSerializer.Protobuf)
            {
                Error error = (Error)ProtobufSerializer.Decode(rawData, typeof(Error), serializer);
                return new PitayaError(error.Code, error.Msg, error.Metadata);
            }

            var jsonStr = Encoding.UTF8.GetString(rawData);
            var json = SimpleJson.SimpleJson.DeserializeObject<Dictionary<string, object>>(jsonStr);

            var code = (string)json["code"];
            var msg = (string)json["msg"];

            Dictionary<string, string> metadata;
            if (json.ContainsKey("metadata"))
            {
                metadata = (Dictionary<string, string>)SimpleJson.SimpleJson.CurrentJsonSerializerStrategy.DeserializeObject(json["metadata"],
                    typeof(Dictionary<string, string>), new Dictionary<string, string>());
            }
            else
            {
                metadata = new Dictionary<string, string>();
            }

            return new PitayaError(code, msg, metadata);
        }

        //-------------------------PRIVATE METHODS------------------------------//
        // ReSharper disable once ParameterOnlyUsedForPreconditionCheck.Local
        private static void CheckClient(IntPtr client)
        {
            if (client == IntPtr.Zero)
                throw new NullReferenceException("invalid client");
        }

        //-----------------------NATIVE CALLBACKS-------------------------------//
        [MonoPInvokeCallback(typeof(NativeAssertCallback))]
        private static void OnAssert(IntPtr e, IntPtr file, int line)
        {
            var eName = Marshal.PtrToStringAnsi(e);
            var fileName = Marshal.PtrToStringAnsi(file);

            Debug.LogAssertion(string.Format("{0}:{1} Failed assertion {2}", fileName, line, eName));
        }

        [MonoPInvokeCallback(typeof(NativeErrorCallback))]
        private static void OnError(IntPtr client, uint rid, IntPtr errorPtr)
        {
            var errBinding = (PitayaBindingError)Marshal.PtrToStructure(errorPtr, typeof(PitayaBindingError));
            PitayaError error;

            if (errBinding.Code == PitayaConstants.PcRcServerError)
            {
                error = CreatePitayaError(errBinding, ClientSerializer(client));
            }
            else
            {
                var code = RcToStr(errBinding.Code);
                error = new PitayaError(code, "Internal Pitaya error");
            }

            MainQueueDispatcher.Dispatch(() =>
            {
                WeakReference reference;
                if (!Listeners.TryGetValue(client, out reference) || !reference.IsAlive) return;
                var listener = reference.Target as IPitayaListener;
                if (listener != null) listener.OnRequestError(rid, error);
            });
        }

        [MonoPInvokeCallback(typeof(NativeRequestCallback))]
        private static void OnRequest(IntPtr client, uint rid, IntPtr respPtr)
        {
            var buffer = (PitayaBuffer)Marshal.PtrToStructure(respPtr, typeof(PitayaBuffer));

            var rawData = new byte[buffer.Len];
            Marshal.Copy(buffer.Data, rawData, 0, (int)buffer.Len);

            MainQueueDispatcher.Dispatch(() =>
            {
                WeakReference reference;
                if (!Listeners.TryGetValue(client, out reference) || !reference.IsAlive) return;
                var listener = reference.Target as IPitayaListener;
                if (listener != null) listener.OnRequestResponse(rid, rawData);
            });
        }


        [MonoPInvokeCallback(typeof(NativeNotifyCallback))]
        private static void OnNotify(IntPtr req, IntPtr error)
        {
            var errBinding = (PitayaBindingError)Marshal.PtrToStructure(error, typeof(PitayaBindingError));
            DLog(string.Format("OnNotify | rc={0}", RcToStr(errBinding.Code)));
        }

        [MonoPInvokeCallback(typeof(NativePushCallback))]
        private static void OnPush(IntPtr client, IntPtr routePtr, IntPtr payloadBufferPtr)
        {
            var route = Marshal.PtrToStringAnsi(routePtr);
            var buffer = (PitayaBuffer)Marshal.PtrToStructure(payloadBufferPtr, typeof(PitayaBuffer));
            var bodyStr = Marshal.PtrToStringAnsi(buffer.Data, (int)buffer.Len);

            WeakReference reference;

            if (!Listeners.TryGetValue(client, out reference) || !reference.IsAlive)
            {
                DLog(string.Format("OnEvent - no listener fond for client ev={0}", client));
                return;
            }

            var listener = reference.Target as IPitayaListener;
            MainQueueDispatcher.Dispatch(() =>
            {
                var serializedBody = Encoding.UTF8.GetBytes(bodyStr);
                if (listener != null) listener.OnUserDefinedPush(route, serializedBody);
            });
        }

        [MonoPInvokeCallback(typeof(NativeEventCallback))]
        private static void OnEvent(IntPtr client, int ev, IntPtr exData, IntPtr arg1Ptr, IntPtr arg2Ptr)
        {
            DLog(string.Format("OnEvent - pinvoke callback START | ev={0} client={1}", EvToStr(ev), client));
            if (arg1Ptr != IntPtr.Zero)
            {
                DLog(string.Format("OnEvent - msg={0}", Marshal.PtrToStringAnsi(arg1Ptr)));
            }

            WeakReference reference;

            if (!Listeners.TryGetValue(client, out reference) || !reference.IsAlive)
            {
                DLog(string.Format("OnEvent - no listener fond for client ev={0}", client));
                return;
            }

            var listener = reference.Target as IPitayaListener;

            MainQueueDispatcher.Dispatch(() =>
            {
                switch (ev)
                {
                    case PitayaConstants.PcEvConnected:
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.Connected);
                        break;
                    case PitayaConstants.PcEvConnectError:
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.FailToConnect);
                        break;
                    case PitayaConstants.PcEvConnectFailed:
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.FailToConnect);
                        break;
                    case PitayaConstants.PcEvDisconnect:
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.Disconnected);
                        break;
                    case PitayaConstants.PcEvKickedByServer:
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.Kicked);
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.Disconnected);
                        break;
                    case PitayaConstants.PcEvUnexpectedDisconnect:
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.Disconnected);
                        break;
                    case PitayaConstants.PcEvProtoError:
                        if (listener != null) listener.OnNetworkEvent(PitayaNetWorkState.Error);
                        break;
                }
                DLog("OnEvent - main thread END");
            });
            DLog("OnEvent - pinvoke callback END");
        }

        [MonoPInvokeCallback(typeof(NativeLogFunction))]
        private static void LogFunction(PitayaLogLevel level, string msg)
        {
            switch (level)
            {
                case PitayaLogLevel.Debug:
                    Debug.Log("[DEBUG] " + msg);
                    break;
                case PitayaLogLevel.Info:
                    Debug.Log("[INFO] " + msg);
                    break;
                case PitayaLogLevel.Warn:
                    Debug.Log("[WARN] " + msg);
                    break;
                case PitayaLogLevel.Error:
                    Debug.Log("[ERROR] " + msg);
                    break;
                case PitayaLogLevel.Disable:
                    // Don't do anything
                    break;
            }
        }

#if (UNITY_IPHONE || UNITY_XBOX360) && !UNITY_EDITOR
        private const string LibName = "__Internal";
#elif (UNITY_ANDROID) && !UNITY_EDITOR
        private const string LibName = "libpitaya-android";
#elif (UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX)
        private const string LibName = "libpitaya-mac";
#elif (UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN)
        private const string LibName = "pitaya-windows";
#else
        private const string LibName = "libpitaya-linux";
#endif

        // ReSharper disable UnusedMember.Local
        [DllImport(LibName, EntryPoint = "tr_uv_tls_set_ca_file")]
        private static extern void NativeSetCertificatePath(string caFile, string caPath);

        [DllImport(LibName, EntryPoint = "pc_unity_lib_init")]
        private static extern void NativeLibInit(int logLevel, string caFile, string caPath, NativeAssertCallback assert, string platform, string buildNumber, string version);

        [DllImport(LibName, EntryPoint = "pc_lib_set_default_log_level")]
        private static extern void NativeLibSetLogLevel(int logLevel);

        [DllImport(LibName, EntryPoint = "pc_client_ev_str")]
        private static extern IntPtr NativeEvToStr(int ev);
        [DllImport(LibName, EntryPoint = "pc_client_rc_str")]
        private static extern IntPtr NativeRcToStr(int rc);

        [DllImport(LibName, EntryPoint = "pc_unity_create")]
        private static extern IntPtr NativeCreate(bool enableTls, bool enablePoll, bool enableReconnect, int connTimeout);
        [DllImport(LibName, EntryPoint = "pc_unity_destroy")]
        private static extern int NativeDestroy(IntPtr client);

        [DllImport(LibName, EntryPoint = "pc_client_connect")]
        private static extern int NativeConnect(IntPtr client, string host, int port, string handsharkOpts);
        [DllImport(LibName, EntryPoint = "pc_client_disconnect")]
        private static extern int NativeDisconnect(IntPtr client);

        [DllImport(LibName, EntryPoint = "pc_unity_request")]
        private static extern int NativeRequest(IntPtr client, string route, string msg, uint cbUid, int timeout, NativeRequestCallback callback, NativeErrorCallback errorCallback);

        [DllImport(LibName, EntryPoint = "pc_unity_binary_request")]
        private static extern int NativeBinaryRequest(IntPtr client, string route, byte[] data, long len, uint cbUid, int timeout, NativeRequestCallback callback, NativeErrorCallback errorCallback);

        [DllImport(LibName, EntryPoint = "pc_string_notify_with_timeout")]
        private static extern int NativeNotify(IntPtr client, string route, string msg, IntPtr exData, int timeout, NativeNotifyCallback callback);
        [DllImport(LibName, EntryPoint = "pc_binary_notify_with_timeout")]
        private static extern int NativeBinaryNotify(IntPtr client, string route, byte[] data, long len, IntPtr exData, int timeout, NativeNotifyCallback callback);
        [DllImport(LibName, EntryPoint = "pc_client_poll")]
        private static extern int NativePoll(IntPtr client);

        [DllImport(LibName, EntryPoint = "pc_client_add_ev_handler")]
        private static extern int NativeAddEventHandler(IntPtr client, NativeEventCallback callback, IntPtr exData, IntPtr destructor);

        [DllImport(LibName, EntryPoint = "pc_client_set_push_handler")]
        private static extern int NativeAddPushHandler(IntPtr client, NativePushCallback callback);

        [DllImport(LibName, EntryPoint = "pc_client_rm_ev_handler")]
        private static extern int NativeRemoveEventHandler(IntPtr client, int handlerId);

        [DllImport(LibName, EntryPoint = "pc_client_conn_quality")]
        private static extern int NativeQuality(IntPtr client);
        [DllImport(LibName, EntryPoint = "pc_client_state")]
        private static extern int NativeState(IntPtr client);

        [DllImport(LibName, EntryPoint = "pc_client_serializer")]
        private static extern IntPtr NativeSerializer(IntPtr client);

        [DllImport(LibName, EntryPoint = "pc_client_free_serializer")]
        private static extern IntPtr NativeFreeSerializer(IntPtr serializer);

        // ReSharper restore UnusedMember.Local

        [DllImport(LibName, EntryPoint = "pc_lib_add_pinned_public_key_from_certificate_string")]
        private static extern int NativeAddPinnedPublicKeyFromCertificateString(string ca_string);

        [DllImport(LibName, EntryPoint = "pc_lib_add_pinned_public_key_from_certificate_file")]
        private static extern int NativeAddPinnedPublicKeyFromCertificateFile(string caPath);

        [DllImport(LibName, EntryPoint = "pc_lib_skip_key_pin_check")]
        private static extern void NativeSkipKeyPinCheck(bool shouldSkip);

        [DllImport(LibName, EntryPoint = "pc_lib_clear_pinned_public_keys")]
        private static extern void NativeClearPinnedPublicKeys();

        [DllImport(LibName, EntryPoint = "pc_unity_init_log_function")]
        private static extern int NativeInitLogFunction(NativeLogFunction fn);
#if UNITY_IPHONE && !UNITY_EDITOR
        [DllImport("__Internal")]
        private static extern string _PitayaGetCFBundleVersion();
#else
        private static string _PitayaGetCFBundleVersion() { return "1"; }
#endif
    }
}
