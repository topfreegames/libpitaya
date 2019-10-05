using System.Text;
using NUnit.Framework;

namespace Pitaya.Tests
{
    public class JsonSerializerTest
    {
        private byte[] _jsonEncoded;
        private string _data;

        [SetUp]
        public void Init()
        {
            _data = "{\"data\":{\"name\":\"test\"}}";
            _jsonEncoded = Encoding.UTF8.GetBytes(_data);
        }

        [Test]
        public void ShouldEncodeMessageSuccessfully()
        {
            var encoded = JsonSerializer.Encode(_data);
            Assert.AreEqual(encoded, _jsonEncoded);
        }

        [Test]
        public void ShouldDecodeMessageSuccessfully()
        {
            var decoded = JsonSerializer.Decode(_jsonEncoded);
            Assert.AreEqual(decoded, _data);
        }

        [Test]
        public void ShouldReturnNullWhenEncodingNullObject()
        {
            Assert.IsNull(JsonSerializer.Encode(null));
        }
    }
}
