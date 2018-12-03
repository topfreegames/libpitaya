using System.Collections;
using System.Threading;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;
using Pitaya.Json;
using Protos;
using SimpleJson;

namespace Pitaya.Tests
{
    public class JsonRequestTest
    {
        private PitayaClient _client;

        private const string ServerHost = "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com";
        private const int ServerPort = 3251;
        private JsonObject _jsonStub;
        private JsonObject _emptyJsonStub;
        private bool _isFinished;
        private string _data;
        private string _emptyData;

        [SetUp]
        public void Init()
        {
            _client = new PitayaClient();
            PitayaClient.SetLogLevel(PitayaLogLevel.Debug);

            _client.Connect(ServerHost, ServerPort);

            _data = "{\"Data\":{\"name\":\"test25\"}}";
            _jsonStub = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(_data);
            _emptyData = "{\"Data\":{}}";
            _emptyJsonStub = (JsonObject)SimpleJson.SimpleJson.DeserializeObject(_emptyData);

            _isFinished = false;

            Thread.Sleep(300);

            // clearing sessiondata
            _client.Request(
                "connector.setsessiondata",
                _emptyData,
                res => {
                    _isFinished = true;
                },
                error => {
                    _isFinished = true;
                }
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
            while(!_isFinished) {
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
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }

            var resp = (JsonObject)SimpleJson.SimpleJson.DeserializeObject((string)response);

            Assert.True(_isFinished);
            Assert.NotNull(response);
            Assert.AreEqual(resp["Code"], 200);
            Assert.AreEqual(resp["Msg"], "success");
        }

        [UnityTest]
        public IEnumerator ShouldGetAndSetJsonObject()
        {
            while(!_isFinished) {
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
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }

            _isFinished = false;
            response = null;

            _client.Request(
                "connector.getsessiondata",
                res => {
                    response = res;
                    _isFinished = true;
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }

            Assert.True(_isFinished);
            Assert.NotNull(response);
            Assert.AreEqual(_data, response);
            Assert.AreEqual(_jsonStub, (JsonObject)SimpleJson.SimpleJson.DeserializeObject((string)response));
        }

        [UnityTest]
        public IEnumerator ShouldReceiveEmptyDataWhenSettingNullRequest()
        {
            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }
            _isFinished = false;

            object response = null;

            _client.Request(
                "connector.setsessiondata",
                res => {
                    response = res;
                    _isFinished = true;
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }

            _isFinished = false;
            response = null;

            _client.Request(
                "connector.getsessiondata",
                res => {
                    response = res;
                    _isFinished = true;
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }

            Assert.True(_isFinished);
            Assert.NotNull(response);
            var responseJson = (JsonObject) SimpleJson.SimpleJson.DeserializeObject((string) response);
            Assert.True(responseJson.ContainsKey("Data"));
        }

        [UnityTest]
        public IEnumerator ShouldReturnBadRequestWhenRequestingToInvalidRoute()
        {
            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }
            _isFinished = false;

            object response = null;

            _client.Request(
                "somewrongroute",
                res => {
                    response = res;
                    _isFinished = true;
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(0.3f);
            }

            Assert.True(_isFinished);
            Assert.NotNull(response);
            Assert.AreEqual(typeof(PitayaError), response.GetType());
            Assert.AreEqual(((PitayaError)response).Code, "PIT-400");
        }
    }
}
