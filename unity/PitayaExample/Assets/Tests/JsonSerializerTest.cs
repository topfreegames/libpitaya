using System.Text;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
    
namespace Pitaya.Tests
{
    public class JsonSerializerTest
    {

        private JObject _jsonStub;
        private byte[] _jsonEncoded;

        [SetUp]
        public void Init()
        {
            const string data = "{\"Data\":{\"name\":\"test\"}}";
            _jsonStub = (JObject)JsonConvert.DeserializeObject(data);
            _jsonEncoded = Encoding.UTF8.GetBytes(data);
        }

        [Test]
        public void ShouldEncodeMessageSuccessfully()
        {
            var encoded = JsonSerializer.Encode(_jsonStub);

            Assert.AreEqual(encoded, _jsonEncoded);
        }

        [Test]
        public void ShouldDecodeMessageSuccessfully()
        {
            var decoded = JsonSerializer.Decode(_jsonEncoded);
        
            Assert.AreEqual(typeof(JObject), decoded.GetType());
            Assert.AreEqual(decoded, _jsonStub);
        }

        [Test]
        public void ShouldNotEncodeMessageIfObjectIsNotJsonObject()
        {
            const string wrongData = "wrong data";

            Assert.Null(JsonSerializer.Encode(wrongData));
        }
    }
}
