using System.Text;
using Newtonsoft.Json;
using NUnit.Framework;
using Protos;
using UnityEngine;

namespace Pitaya.Tests
{
    public class JsonSerializerTest
    {

        private SessionData _jsonStub;
        private byte[] _jsonEncoded;
        private string _data;

        [SetUp]
        public void Init()
        {
            _data = _data = "{\"Data\":\"{\\\"name\\\":\\\"test\\\"}\"}";
            _jsonStub = JsonConvert.DeserializeObject<SessionData>(_data);
            _jsonEncoded = Encoding.UTF8.GetBytes(_data);
        }

        [Test]
        public void ShouldEncodeMessageSuccessfully()
        {
            var encoded = new JsonSerializer().Encode(_jsonStub);
            Assert.AreEqual(_jsonEncoded, encoded);
        }

        [Test]
        public void ShouldDecodeMessageSuccessfully()
        {
            var decoded = new JsonSerializer().Decode<SessionData>(_jsonEncoded);
        
            Assert.AreEqual(typeof(SessionData), decoded.GetType());
            Assert.AreEqual(decoded, _jsonStub);
        }
    }
}
