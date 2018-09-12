using Google.Protobuf;
using NUnit.Framework;
using Protos;

namespace Pitaya.Tests
{
    public class ProtobufSerializerTest
    {

        private Response _responseStub;
        private byte[] _responseEncoded;

        [SetUp]
        public void Init()
        {
            _responseStub = new Response
            {
                Code = 1,
                Msg = "Fake message"
            };

            _responseEncoded = _responseStub.ToByteArray();

        }

        [Test]
        public void ShouldStoreDescriptorWhenCreatingProtobuf()
        {
//            Assert.NotNull(_p.GetDescriptor());
        }

        [Test]
        public void ShouldGetDescriptorReturnCorrectDescriptor()
        {
//            _p = new ProtobufSerializer(SessionData.Descriptor);
//            var md = _p.GetDescriptor();
//
//            Assert.AreEqual(SessionData.Descriptor, md);
//            Assert.AreNotEqual(Response.Descriptor, md);
        }

        [Test]
        public void ShouldEncodeMessageSuccessfully()
        {
//            var encoded = ProtobufSerializer.Encode(_responseStub);
//
//            Assert.AreEqual(encoded, _responseEncoded);
//
//            _p = new ProtobufSerializer(SessionData.Descriptor);
//            encoded = _p.Encode(_sessionStub);
//
//            Assert.AreEqual(encoded, _sessionEncoded);
        }

        [Test]
        public void ShouldDecodeMessageSuccessfully()
        {
            var obj = ProtobufSerializer.Decode(_responseEncoded, typeof(Response), PitayaSerializer.Protobuf);

            Assert.AreEqual(obj.GetType(), typeof(Response));
            Assert.AreEqual(obj, _responseStub);
        }

        [Test]
        public void ShouldNotEncodeMessageIfObjectPassedIsNotIMessage()
        {
//            const string message = "wrong data";
//
//            Assert.Null(_p.Encode(message));
        }

        [Test]
        public void ShouldNotDecodeMessageIfThereIsNoDescriptor()
        {
//            _p = new ProtobufSerializer();
//
//            Assert.Null(_p.Decode(_responseEncoded));
//            Assert.Null(_p.Decode(_sessionEncoded));
        }
    }
}
