using System;
using System.Text;
using NUnit.Framework;
using Protos;

namespace Pitaya.Tests
{
    public class ProtobufSerializerTest
    {

        private Response _responseStub;
        private byte[] _responseEncoded;
        private byte[] _respondeMessageEncodeJson;
        
        private BigMessage _bigMessageStub;
        private byte[] _bigMessageEncodedProtobuf;
        private byte[] _bigMessageEncodedJson;

        [SetUp]
        public void Init()
        {
            _responseStub = new Response
            {
                Code = 1,
                Msg = "Fake message"
            };
            
            var player = new PlayerResponse
            {
                PublicId = "publicId",
                AccessToken = "Token",
                Name = "PlayerName",
                Items = { "item1", "item2"},
                Health = 2.10
            };
            var npc = new NPC
            {
                Name = "NPC_NAME",
                PublicId = "NPC_publicID",
                Health = 10.3
            };
            var bigResponse = new BigMessageResponse
            {
                Player = player,
                Npcs = {{ "npc_key", npc}},
                Chests = { "chest1", "chest2"}
            };
            _bigMessageStub = new BigMessage
            {
                Code = "22",
                Response = bigResponse
            };
            _respondeMessageEncodeJson = Encoding.UTF8.GetBytes("{\"code\": 1,\"msg\":\"Fake message\"}");
            _bigMessageEncodedJson = Encoding.UTF8.GetBytes("{\"code\":\"22\",\"response\":{\"player\":{\"publicId\":\"publicId\",\"accessToken\":\"Token\",\"name\":\"PlayerName\",\"items\":[\"item1\",\"item2\"],\"health\":2.1},\"npcs\":{\"npc_key\":{\"name\":\"NPC_NAME\",\"health\":10.3,\"publicId\":\"NPC_publicID\"}},\"chests\":[\"chest1\",\"chest2\"]}}");
            _bigMessageEncodedProtobuf = new byte[] {0x0A, 0x02, 0x32, 0x32, 0x12, 0x74, 0x0A, 0x34, 0x0A, 0x08, 0x70, 0x75, 0x62, 0x6C, 0x69, 0x63, 0x49, 0x64, 0x12, 0x05, 0x54, 0x6F, 0x6B, 0x65, 0x6E, 0x1A, 0x0A, 0x50, 0x6C, 0x61, 0x79, 0x65, 0x72, 0x4E, 0x61, 0x6D, 0x65, 0x22, 0x05, 0x69, 0x74, 0x65, 0x6D, 0x31, 0x22, 0x05, 0x69, 0x74, 0x65, 0x6D, 0x32, 0x29, 0xCD, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x00, 0x40, 0x12, 0x2C, 0x0A, 0x07, 0x6E, 0x70, 0x63, 0x5F, 0x6B, 0x65, 0x79, 0x12, 0x21, 0x0A, 0x08, 0x4E, 0x50, 0x43, 0x5F, 0x4E, 0x41, 0x4D, 0x45, 0x11, 0x9A, 0x99, 0x99, 0x99, 0x99, 0x99, 0x24, 0x40, 0x1A, 0x0C, 0x4E, 0x50, 0x43, 0x5F, 0x70, 0x75, 0x62, 0x6C, 0x69, 0x63, 0x49, 0x44, 0x1A, 0x06, 0x63, 0x68, 0x65, 0x73, 0x74, 0x31, 0x1A, 0x06, 0x63, 0x68, 0x65, 0x73, 0x74, 0x32};


            _responseEncoded = new ProtobufSerializer(ProtobufSerializer.SerializationFormat.Protobuf).Encode(_responseStub);

        }
        
        private static bool ByteArrayCompare(byte[] a1, byte[] a2)
        {
            var s1 = Convert.ToBase64String(a1);
            var s2 = Convert.ToBase64String(a2);
            return s1 == s2;
        }


        [Test]
        public void ShouldEncodeProtobufMessageSuccessfully()
        {
            var output = new ProtobufSerializer(ProtobufSerializer.SerializationFormat.Protobuf).Encode(_bigMessageStub);
            Assert.True(output.Length > 0);
            Assert.True(ByteArrayCompare(output, _bigMessageEncodedProtobuf));

        }
        

        [Test]
        public void ShouldDecodeProtobufMessageSuccessfully()
        {
            var obj = new ProtobufSerializer(ProtobufSerializer.SerializationFormat.Protobuf).Decode<Response>(_responseEncoded);

            Assert.True(typeof(Response) == obj.GetType());
            Assert.True(IsResponseEquals(obj, _responseStub));
        }
        
        [Test]
        public void ShouldEncodeJsonMessageSuccessfully()
        {
            var output = new ProtobufSerializer(ProtobufSerializer.SerializationFormat.Json).Encode(_bigMessageStub);
            Assert.True(output.Length > 0);
        }
        

        [Test]
        public void ShouldDecodeJsonMessageSuccessfully()
        {
            var obj = new ProtobufSerializer(ProtobufSerializer.SerializationFormat.Json).Decode<Response>(_respondeMessageEncodeJson);

            Assert.AreEqual(typeof(Response), obj.GetType());
            Assert.AreEqual(obj, _responseStub);
        }

        private static bool IsResponseEquals(Response obj1, Response obj2)
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
