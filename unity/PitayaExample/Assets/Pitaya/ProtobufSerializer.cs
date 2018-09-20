using System;
using System.IO;
using System.Text;
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
            
//            var jsf = new JsonFormatter(new JsonFormatter.Settings(true));
//            var jsonString = jsf.Format(message);
//            
//            return Encoding.UTF8.GetBytes(jsonString);
            return null;
        }
        
        public static IExtensible Decode(byte[] buffer, Type type, PitayaSerializer serializer)
        {
            if (PitayaSerializer.Protobuf == serializer)
            {
                var res = (IExtensible) Activator.CreateInstance(type);
//                var merged = (IExtensible) Serializer.NonGeneric.Merge(new MemoryStream(buffer), res);
                
                return (IExtensible) Serializer.Deserialize(type, new MemoryStream(buffer));
            }
            
//            var stringified = Encoding.UTF8.GetString(data);
//            return (IMessage)SimpleJson.SimpleJson.DeserializeObject(stringified, type);  
            return null;
        }
    }
}
