using System.Text;
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
            _data = "{\"data\":{\"name\":\"test\"}}";
            _jsonStub = JsonUtility.FromJson<SessionData>(_data);
            _jsonEncoded = Encoding.UTF8.GetBytes(_data);
        }

        [Test]
        public void ShouldEncodeMessageSuccessfully()
        {
//            var encoded = JsonSerializer.Encode(_data);
//
//            Assert.AreEqual(encoded, _jsonEncoded);
        }

        [Test]
        public void ShouldDecodeMessageSuccessfully()
        {
//            var decoded = JsonSerializer.Decode(_jsonEncoded);
//        
//            Assert.AreEqual(typeof(JObject), decoded.GetType());
//            Assert.AreEqual(decoded, _jsonStub);
        }

        [Test]
        public void ShouldNotEncodeMessageIfObjectIsNotJsonObject()
        {
//            const string wrongData = "wrong data";
//
//            Assert.Null(JsonSerializer.Encode(wrongData));
        }
    }
}
