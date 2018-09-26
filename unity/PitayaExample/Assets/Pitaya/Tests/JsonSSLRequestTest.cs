﻿using System.Collections;
using System.Threading;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
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
        private JObject _jsonStub;
        private JObject _emptyJsonStub;
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
            _jsonStub = (JObject)JsonConvert.DeserializeObject(_data);
            _emptyData = "{\"Data\":{}}";
            _emptyJsonStub = (JObject)JsonConvert.DeserializeObject(_emptyData);

            _isFinished = false;

            // clearing sessiondata
            _client.Request(
                "connector.setsessiondata",
                _emptyJsonStub,
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
                _jsonStub,
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
            Assert.AreEqual(typeof(JObject), response.GetType());
            Assert.AreEqual((int)((JObject)response)["Code"], 200);
            Assert.AreEqual((string)((JObject)response)["Msg"], "success");
        }

        [UnityTest]
        public IEnumerator ShouldGetAndSetJsonObject()
        {
            object response = null;

            _client.Request(
                "connector.setsessiondata",
                _jsonStub,
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
            Assert.AreEqual(response, _jsonStub);
            Assert.AreEqual(JsonConvert.SerializeObject(response), _data);
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
            Assert.AreEqual(response, _emptyJsonStub);
            Assert.AreEqual(JsonConvert.SerializeObject(response), _emptyData);
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