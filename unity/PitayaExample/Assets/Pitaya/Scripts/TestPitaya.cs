/**
 * Copyright (c) 2014-2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

using System;
using System.Collections;
using SimpleJson;
using UnityEngine;

namespace Pitaya.Scripts
{
    public class TestPitaya : MonoBehaviour
    {
        private PitayaClient _client;
        private PitayaClient _clienUno;
        private PitayaClient[] _list;

        private readonly System.Text.StringBuilder _log = new System.Text.StringBuilder("App Start\n\n\n");

        private void DLog(object data)
        {
            _log.AppendLine(data.ToString());
            Debug.Log(data, null);
        }

        /// <summary>
        /// cleanup the pomelo client
        /// NOTE it's not the best practice since it would not invoke on ios
        /// </summary>
        ///
        private void OnApplicationQuit()
        {
            if(_client != null)
            {
                DLog("client ready to destroy");
                _client.Dispose();
                _client = null;
                DLog("destroy client successful");
            }
            else
            {
                DLog("no client to destroy!");
            }
        }

        private void PrintClientState()
        {
            DLog("CurrentStatte " + _client.State);
        }

        private void Start()
        {
            _client = new PitayaClient();
            PrintClientState();

            PitayaClient.SetLogLevel(PitayaLogLevel.Error);

            _client.NetWorkStateChangedEvent += networkState => {
                switch(networkState) {
                    case PitayaNetWorkState.Connected:
                        PrintClientState();
                        OnConnect();
                        break;
                    case PitayaNetWorkState.Disconnected:
                        PrintClientState();
                        DLog("client1 disconnected");
                        break;
                    case PitayaNetWorkState.FailToConnect:
                        PrintClientState();
                        DLog("client failed to connect");
                        break;
                    case PitayaNetWorkState.Kicked:
                        PrintClientState();
                        DLog("client Kicked");
                        break;
                    case PitayaNetWorkState.Closed:
                        break;
                    case PitayaNetWorkState.Connecting:
                        break;
                    case PitayaNetWorkState.Timeout:
                        break;
                    case PitayaNetWorkState.Error:
                        break;
                    default:
                        throw new ArgumentOutOfRangeException(nameof(networkState), networkState, null);
                }
            };

            _client.Connect(ServerHost(), ServerPort());
            PrintClientState();
        }

        private void OnConnect()
        {
            StartCoroutine(KeepTestClient());
        }

        private IEnumerator KeepTestClient()
        {
            // JSONOBJECT EXAMPLES (RUN ON PORT 3251)

            // listen to route and receive data from push route

            _client.OnRoute("some.push.route", body => {
                DLog("[some.push.route] PUSH RESPONSE = " + body);
            });

            _client.Request("connector.sendpush",
                res => {
                    Debug.Log($"[connector.sendpush] response: {res}");
                },
                error => {
                    Debug.Log($"[connector.sendpush] error: {error}");
                }
            );

            // get and set session data (send and receive JsonObject)

            var isFinished = false;
            const string data = "{\"Data\": { \"name\": \"test\" }}";
            var obj = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(data);

            _client.Request("connector.setsessiondata",
                obj,
                res => {
                    isFinished = true;
                    DLog($"[connector.setsessiondata] - response={res}");
                },
                error => {
                    isFinished = true;
                    DLog($"[connector.setsessiondata] ERROR - error-code={error.Code} metadata={error.Metadata}");
                }
            );

            while(!isFinished) {
                yield return new WaitForSeconds(1);
            }

            isFinished = false;

            _client.Request("connector.getsessiondata",
                res => {
                    isFinished = true;
                    DLog($"[connector.getsessiondata] - response={res}");
                },
                error => {
                    isFinished = true;
                    DLog($"[connector.getsessiondata] ERROR - error-code={error.Code} metadata={error.Metadata}");
                }
            );

            while(!isFinished) {
                yield return new WaitForSeconds(1);
            }

            // END JSONOBJECT EXAMPLES

            // PROTOBUF EXAMPLES (RUN ON PORT 3351)

//            // set a serializer for the route response (set with which Protobuf the response is going to be decoded)
//            _client.SubscribeSerializer("connector.setsessiondata", new ProtobufSerializer(Response.Descriptor));
//            _client.SubscribeSerializer("connector.getbigmessage", new ProtobufSerializer(BigMessage.Descriptor));
//            _client.SubscribeSerializer("connector.getsessiondata", new ProtobufSerializer(SessionData.Descriptor));
//            _client.SubscribeSerializer("connector.notifysessiondata", new ProtobufSerializer(SessionData.Descriptor));
//
//            var sd = new SessionData
//            {
//                Data = "fake data"
//            };
//
//            // notify server with protobuf data
//            _client.Notify("connector.notifysessiondata", sd);
//
//            // get and set session data (send and receive Protobuf)
//
//            var isFinished = false;
//            const string data = "{\"Data\": { \"name\": \"test\" }}";
//            var obj = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(data);
//
//            _client.Request("connector.setsessiondata",
//                obj,
//                res => {
//                    isFinished = true;
//                    DLog($"[connector.setsessiondata] - response={res}");
//                },
//                error => {
//                    isFinished = true;
//                    DLog($"[connector.setsessiondata] ERROR - error-code={error.Code} metadata={error.Metadata}");
//                }
//            );
//
//            while(!isFinished) {
//                yield return new WaitForSeconds(1);
//            }
//
//            isFinished = false;
//
//            // GetSessionData is not implemented yet in go server
//            _client.Request("connector.getsessiondata",
//                res => {
//                    isFinished = true;
//                    DLog($"[connector.getsessiondata] - response={res}");
//                },
//                error => {
//                    isFinished = true;
//                    DLog($"[connector.getsessiondata] ERROR - error-code={error.Code} metadata={error.Metadata}");
//                }
//            );
//
//            while(!isFinished) {
//                yield return new WaitForSeconds(1);
//            }

            // END PROTOBUF EXAMPLES

            yield return new WaitForSeconds(1);

            _client.Disconnect();
            DLog("Client disconnected");
        }

        private string ServerHost()
        {
#if UNITY_EDITOR
            const string host = "127.0.0.1";
#else
            const string host = "10.0.22.57";
#endif
            DLog("server host : " + host);
            return host;
        }

        private static int ServerPort()
        {
            return 3251;
        }
    }

}
