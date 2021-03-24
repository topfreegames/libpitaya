﻿using System.Collections;
using System.Threading;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;
using Pitaya.SimpleJson;

namespace Pitaya.Tests
{
    [TestFixture, Category("integration")]
    public class PitayaClientPushTest
    {
        private PitayaClient _client;
        private const string ServerHost = "libpitaya-tests.tfgco.com";
        private const int ServerPort = 3251;
        private Thread _mainThread;

        [SetUp]
        public void Setup()
        {
            _mainThread = Thread.CurrentThread;
            _client = new PitayaClient();
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
        public IEnumerator ShouldClientBeAbleToListenToARouteAndReceiveData()
        {
            var receivedPushResponse = false;
            object response = null;

            const string expectedResponse = "{\"key1\":10,\"key2\":true}";

            _client.Connect(ServerHost, ServerPort);

            _client.OnRoute("some.push.route",
                res => {
                    receivedPushResponse = true;
                    response = res;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            var requestFinished = false;
            object sendPushResponse = null;

            _client.Request(
                "connector.sendpush",
                res => {
                    requestFinished = true;
                    sendPushResponse = res;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    requestFinished = true;
                    sendPushResponse = error;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            while(!requestFinished) {
                yield return new WaitForSeconds(0.5f);
            }

            var resp = (JsonObject)SimpleJson.SimpleJson.DeserializeObject((string) sendPushResponse);

            Assert.True(receivedPushResponse);
            Assert.NotNull(response);
            Assert.AreEqual(response, expectedResponse);
            Assert.NotNull(sendPushResponse);
            Assert.AreEqual(resp["Code"], 200);
            Assert.AreEqual(resp["Msg"], "Ok");
        }

        [UnityTest]
        public IEnumerator ShouldClientNotReceiveDataFromAnUnlistenedRoute()
        {
            var receivedPushResponse = false;
            object response = null;
            const string pushRoute = "some.push.route";
            bool connectFinished = false;
            bool connectOk = false;

            _client.Connect(ServerHost, ServerPort);

            _client.NetWorkStateChangedEvent += (ev, error) =>
            {
                connectFinished = true;
                if (ev == PitayaNetWorkState.Connected)
                    connectOk = true;
            };

            while (!connectFinished)
            {
                yield return new WaitForSeconds(0.5f);
            }

            Assert.True(connectOk);

            _client.OnRoute(pushRoute,
                res => {
                    receivedPushResponse = true;
                    response = res;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            _client.OffRoute(pushRoute);

            var requestFinished = false;

            _client.Request(
                "connector.sendpush",
                res => {
                    requestFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                },
                error => {
                    requestFinished = true;
                    Assert.AreEqual(_mainThread, Thread.CurrentThread);
                }
            );

            while(!requestFinished)
            {
                yield return new WaitForSeconds(0.5f);
            }

            Assert.False(receivedPushResponse);
            Assert.Null(response);
        }
    }
}
