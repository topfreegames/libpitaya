using System.Collections;
using System.Threading;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;
using Pitaya.SimpleJson;

namespace Pitaya.Tests
{
    public class JsonSSLRequestTest
    {

        private PitayaClient _client;

        private const string ServerHost = "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com";

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

            _client = new PitayaClient("myCA.pem");
            PitayaClient.SetLogLevel(PitayaLogLevel.Debug);

            _client.Connect(ServerHost, ServerPort);

            Thread.Sleep(200);

            _data = "{\"Data\":{\"name\":\"test25\"}}";
            _jsonStub = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(_data);
            _emptyData = "{\"Data\":{}}";
            _emptyJsonStub = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(_emptyData);

            _isFinished = false;

            // clearing sessiondata
            _client.Request(
                "connector.setsessiondata",
                _emptyData,
                res => { _isFinished = true; },
                error => { _isFinished = true; }
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
            while (!_isFinished)
            {
                yield return new WaitForSeconds(0.3f);
            }

            _isFinished = false;

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
            while (!_isFinished)
            {
                yield return new WaitForSeconds(0.3f);
            }

            _isFinished = false;

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

            _isFinished = false;

        }

        [UnityTest]
        public IEnumerator ShouldReceiveEmptyDataWhenSettingNullRequest()
        {
            while (!_isFinished)
            {
                yield return new WaitForSeconds(0.3f);
            }

            _isFinished = false;

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
            while (!_isFinished)
            {
                yield return new WaitForSeconds(0.3f);
            }

            _isFinished = false;

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
