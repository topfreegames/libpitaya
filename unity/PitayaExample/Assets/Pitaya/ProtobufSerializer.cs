using System;
using System.Text;
using Google.Protobuf;
using UnityEngine;

namespace Pitaya
{
    public static class ProtobufSerializer
    {
        public static byte[] Encode(IMessage message, PitayaSerializer serializer)
        {
            if (PitayaSerializer.Protobuf == serializer)
            {
                return message.ToByteArray();
            }
            
            
            var jsf = new JsonFormatter(new JsonFormatter.Settings(true));
            var jsonString = jsf.Format(message);
            
            return Encoding.UTF8.GetBytes(jsonString);

        }
        
        public static IMessage Decode(byte[] buffer, Type type, PitayaSerializer serializer)
        {
            var res = (IMessage) Activator.CreateInstance(type);
            if (PitayaSerializer.Protobuf == serializer)
            {   
                res.MergeFrom(buffer);
                return res;

            }
            var stringified = Encoding.UTF8.GetString(buffer);
            
            return JsonParser.Default.Parse(stringified, res.Descriptor);

//            return res;
//            var parser = new MessageParser();
            
            return (IMessage) JsonUtility.FromJson(stringified, type);
//            return (IMessage) JsonConvert.DeserializeObject(stringified, type);
        }

        public static T FromJson<T>(string json) where T : IMessage, new()
        {
            return JsonParser.Default.Parse<T>(json);
        }
        
    }
}
