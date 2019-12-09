using System.Collections;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Pitaya.Tests
{
    public class PitayaMetricsTest
    {
        const string ServerHost = "libpitaya-tests.tfgco.com";
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

        [UnityTest]
        public IEnumerator StatsShouldBeReportedAtEndOfConnection()
        {
            bool statsCalled = false;

            _client = new PitayaClient(new PitayaMetrics.Config(stats => { statsCalled = true; }));
            
            var called = false;
            var connectionState = PitayaNetWorkState.Disconnected;

            _client.NetWorkStateChangedEvent += (networkState, error) => 
            {
                called = true;
                connectionState = networkState;
            };

            _client.Connect(ServerHost, ServerPort);

            while (!called)
            {
                yield return new WaitForSeconds(0.2f);
            }

            Assert.True(called);
            Assert.AreEqual(connectionState, PitayaNetWorkState.Connected);
            
            _client.Disconnect();
            yield return new WaitForSeconds(0.2f);
            Assert.IsTrue(statsCalled);
        }
    }
}
