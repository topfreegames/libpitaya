using System.Collections;
using NUnit.Framework;
using Protos;
using UnityEngine;
using UnityEngine.TestTools;

namespace Pitaya.Tests
{
    public class ProtobufRequestTest
    {
    
        private PitayaClient _client;

        private const string ServerHost = "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com";
        private const int ServerPort = 3351;
        private Response _response;
        private SessionData _sessionData;
        private BigMessage _bigMessage;
        private bool _isFinished;

        [SetUp]
        public void Init()
        {
            _isFinished = false;
            PitayaClient.SetLogLevel(PitayaLogLevel.Debug);

            _client = new PitayaClient();
            _client.Connect(ServerHost, ServerPort);

            _sessionData = new SessionData
            {
                Data = "fake data"
            };
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
        public IEnumerator ShouldReturnResponseInSubscriberFormat()
        {

            object response;
            

            response = null;
            _isFinished = false;

            _client.Request<Response>("connector.setsessiondata",
                _sessionData,
                (res) => {
                    response = res;
                    _isFinished = true;
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(1);
            }

            Assert.NotNull(response);
            Assert.AreEqual(typeof(Response), response.GetType());

        }
        
        [UnityTest]
        public IEnumerator ShoulSendEmptyRequest()
        {

            Response response = null;
            
            var sessionData = new SessionData();
            
            _isFinished = false;

            _client.Request<Response>("connector.setsessiondata",
                sessionData,
                (res) => {
                    response = res;
                    _isFinished = true;
                },
                error => {
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(1);
            }

            Assert.NotNull(response);
            Assert.AreEqual(typeof(Response), response.GetType());
            Assert.AreEqual(response.Code , 200);

        }
        
        [UnityTest]
        public IEnumerator ShouldReceiveAnError()
        {

            object response;
            

            response = null;
            _isFinished = false;

            _client.Request<Response>("invalid.route",
                _sessionData,
                (res) => {
                    response = res;
                    _isFinished = true;
                },
                error => {
                    response = error;
                    _isFinished = true;
                }
            );

            while(!_isFinished) {
                yield return new WaitForSeconds(1);
            }

            Assert.NotNull(response);
            Assert.AreEqual(typeof(PitayaError), response.GetType());
            Assert.AreEqual("PIT-404", ((PitayaError)response).Code);

        }
        
        [UnityTest]
        public IEnumerator ShoulReceiveAPushNotification()
        {
            object response = null;
            _isFinished = false;
            
            _client.OnRoute<SessionData>("some.push.route", data => { 
                response = data;
                _isFinished = true; 
            });

            _client.Notify("connector.notifysessiondata", _sessionData);

            while(!_isFinished) {
                yield return new WaitForSeconds(1);
            }

            Assert.NotNull(response);
            Assert.AreEqual(typeof(SessionData), response.GetType());
            Assert.AreEqual(((SessionData)response).Data, "THIS IS THE SESSION DATA");

        }
    }
}
