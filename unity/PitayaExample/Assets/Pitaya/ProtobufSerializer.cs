using System;
using System.IO;
using System.Text;
using Newtonsoft.Json;
using ProtoBuf;

namespace Pitaya
{
    public static class ProtobufSerializer
    {
        public static byte[] Encode(IExtensible message, PitayaSerializer serializer)
        {
            if (PitayaSerializer.Protobuf == serializer)
            {
                var stream = new MemoryStream();
                Serializer.Serialize(stream, message);
                return stream.ToArray();
            }
            
            
            var jsonString = JsonConvert.SerializeObject(message);
            return Encoding.UTF8.GetBytes(jsonString);
        }
        
        public static IExtensible Decode(byte[] buffer, Type type, PitayaSerializer serializer)
        {
            if (PitayaSerializer.Protobuf == serializer)
            {   
                return (IExtensible) Serializer.Deserialize(type, new MemoryStream(buffer));
            }
            
            var stringified = Encoding.UTF8.GetString(buffer);
            return (IExtensible) JsonConvert.DeserializeObject(stringified, type);
        }
    }
}
