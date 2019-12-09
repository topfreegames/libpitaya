using System;
using System.Collections;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Pitaya.Tests
{
    public class PitayaMetricsTest
    {
        const string ServerHost = "libpitaya-tests.tfgco.com";
        const string GetSessionDataRoute = "connector.getsessiondata";
        const int ServerPort = 3251;

        PitayaClient _client;

        [SetUp]
        public void Setup() { } 

        [TearDown]
        public void TearDown()
        {
            if (_client == null) return;
            _client.Disconnect();
            _client.Dispose();
            _client = null;
        }

        static IEnumerator Connect(PitayaClient client)
        {
            var called = false;
            var connectionState = PitayaNetWorkState.Disconnected;

            client.NetWorkStateChangedEvent += (networkState, error) => 
            {
                called = true;
                connectionState = networkState;
            };

            client.Connect(ServerHost, ServerPort);

            while (!called)
            {
                yield return new WaitForSeconds(0.2f);
            }

            Assert.True(called);
            Assert.AreEqual(connectionState, PitayaNetWorkState.Connected);
        }

        [UnityTest]
        public IEnumerator StatsShouldBeReportedAtEndOfConnection()
        {
            bool statsCalled = false;

            _client = new PitayaClient(new PitayaMetrics.Config(stats =>
            {
                statsCalled = true;
                Assert.Equals(stats.ConnectionFinishReason, "UserRequest");
            }));
            yield return Connect(_client);
            _client.Disconnect();
            yield return new WaitForSeconds(0.2f);
            Assert.IsTrue(statsCalled);
        }

        [UnityTest]
        public IEnumerator PingStatsShouldBeReportedOnlyWhenRouteIsCalled()
        {
            {
                bool statsCalled = false;

                _client = new PitayaClient(new PitayaMetrics.Config(
                    stats =>
                    {
                        statsCalled = true;
                        Assert.Less(Math.Abs(stats.PingAverage), 0.0001);
                        Assert.Less(Math.Abs(stats.PingStdDeviation), 0.0001);
                        Assert.Equals(stats.PingTotal, 0);
                        Assert.Equals(stats.PingLoss, 0);
                    },
                    GetSessionDataRoute
                ));

                yield return Connect(_client);
                _client.Disconnect();
                yield return new WaitForSeconds(0.3f);
                Assert.IsTrue(statsCalled);
            }
        }
    }
}
