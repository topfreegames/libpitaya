using System;
using NUnit.Framework;
using Protos;
using ProtoBuf;

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
            

            _responseEncoded = ProtobufSerializer.Encode(_responseStub, PitayaSerializer.Protobuf);

        }


        [Test]
        public void ShouldEncodeMessageSuccessfully()
        {
            var output = ProtobufSerializer.Encode(_responseStub, PitayaSerializer.Protobuf);
            Assert.True(output.Length > 0);
            Assert.True(output.Length > 0);

        }
        

        [Test]
        public void ShouldDecodeMessageSuccessfully()
        {
            var obj = ProtobufSerializer.Decode(_responseEncoded, typeof(Response), PitayaSerializer.Protobuf);

            Assert.True(typeof(Response) == obj.GetType());
            Assert.AreEqual(((Response)obj).Code, _responseStub.Code);
            Assert.AreEqual(((Response)obj).Msg, _responseStub.Msg);
        }

        private bool IsResponseEquals(Response obj1, Response obj2)
        {
            if (obj1 == obj2)
            {
                return true;
            }

            if (obj1 == null || obj2 == null)
            {
                return false;
            }

            return obj1.Code == obj2.Code && obj1.Msg == obj2.Msg;
        } 
    }
}
