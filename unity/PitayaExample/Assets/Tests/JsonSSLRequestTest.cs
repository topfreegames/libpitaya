using System.Collections;
using System.Threading;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
using Protos;
using SimpleJson;
using UnityEngine;
using UnityEngine.TestTools;

namespace Pitaya.Tests
{
    public class JsonSSLRequestTest
    {
    
        private PitayaClient _client;

#if UNITY_EDITOR
        private const string ServerHost = "127.0.0.1";
#else
        private const string ServerHost = "10.0.22.57";
#endif
        private const int ServerPort = 3252;
        private JsonObject _jsonStub;
        private JsonObject _emptyJsonStub;
        private bool _isFinished;
        private string _data;
        private string _emptyData;
        private Thread _mainThread;

        [SetUp]
        public void Init()
        {
            // PitayaBinding.AddPinnedPublicKeyFromCaFile("client-ssl.localhost.crt");
            _mainThread = Thread.CurrentThread;
            PitayaBinding.SkipKeyPinCheck(true);

            _client = new PitayaClient("ca.crt");
            PitayaClient.SetLogLevel(PitayaLogLevel.Debug);
            
            _client.Connect(ServerHost, ServerPort);

            _data = "{\"Data\":{\"name\":\"test25\"}}";
            _jsonStub = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(_data);
            _emptyData = "{\"Data\":{}}";
            _emptyJsonStub = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(_emptyData);

            _isFinished = false;

            // clearing sessiondata
            _client.Request(
                "connector.setsessiondata",
                _emptyData,
                res => {},
                error => {}
            );
        }

        [TearDown]
        public void TearDown()
        {
            if (_client == null) return;
            _client.Disconnect();
            _client.Dispose();
            _client = null;
        }

        [UnityTest]
        public IEnumerator ShouldReturnJsonObjectWhenNoSubscriberIsAdded()
        {
            object response = null;

            _client.Request(
                "connector.setsessiondata",
                _data,
                res => {
                    response = res;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    response = error;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }    
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(1);
            }

            var resp = (JsonObject)SimpleJson.SimpleJson.DeserializeObject((string) response);
            
            Assert.True(_isFinished);
            Assert.NotNull(response);
            Assert.AreEqual(resp["Code"], 200);
            Assert.AreEqual(resp["Msg"], "success");
        }

        [UnityTest]
        public IEnumerator ShouldGetAndSetJsonObject()
        {
            object response = null;

            _client.Request(
                "connector.setsessiondata",
                _data,
                res => {
                    response = res;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    response = error;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(1);
            }

            _isFinished = false;
            response = null;

            _client.Request(
                "connector.getsessiondata",
                res => {
                    response = res;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    response = error;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(1);
            }

            Assert.True(_isFinished);
            Assert.NotNull(response);
            Assert.AreEqual(response, _data);
            Assert.AreEqual(SimpleJson.SimpleJson.DeserializeObject((string)response), _jsonStub);
        }

        [UnityTest]
        public IEnumerator ShouldReceiveEmptyDataWhenSettingNullRequest()
        {
            object response = null;

            _client.Request(
                "connector.setsessiondata",
                res => {
                    response = res;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    response = error;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.5f);
            }

            _isFinished = false;
            response = null;

            _client.Request(
                "connector.getsessiondata",
                res => {
                    response = res;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    response = error;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.5f);
            }

            Assert.True(_isFinished);
            Assert.NotNull(response);
            Assert.AreEqual(response, _emptyData);
            Assert.AreEqual(SimpleJson.SimpleJson.DeserializeObject((string)response), _emptyJsonStub);
        }

        [UnityTest]
        public IEnumerator ShouldReturnBadRequestWhenRequestingToInvalidRoute()
        {
            object response = null;

            _client.Request(
                "somewrongroute",
                res => {
                    response = res;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    response = error;
                    _isFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.5f);
            }

            Assert.True(_isFinished);
            Assert.NotNull(response);
            Assert.AreEqual(typeof(PitayaError), response.GetType());
            Assert.AreEqual(((PitayaError)response).Code, "PIT-400");
        }
    }
}
