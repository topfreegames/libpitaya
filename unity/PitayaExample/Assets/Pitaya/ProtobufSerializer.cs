using System;
using System.Text;
using Google.Protobuf;
using UnityEngine;

namespace Pitaya
{
    public static class ProtobufSerializer
    {
        private static byte[] emptyByteArray = new byte[0];

        public static byte[] Encode(IMessage message, PitayaSerializer serializer)
        {
            if (message == null)
            {
                Debug.LogError("Message should not be null, returning empty byte array");
                return emptyByteArray;
            }

            if (serializer == PitayaSerializer.Protobuf)
            {
                return message.ToByteArray();
            }

            var jsf = new JsonFormatter(new JsonFormatter.Settings(true));
            var jsonString = jsf.Format(message);

            return Encoding.UTF8.GetBytes(jsonString);
        }

        public static IMessage Decode(byte[] buffer, Type type, PitayaSerializer serializer)
        {
            if (buffer == null)
            {
                buffer = emptyByteArray;
            }

            var res = (IMessage) Activator.CreateInstance(type);
            if (PitayaSerializer.Protobuf == serializer)
            {
                res.MergeFrom(buffer);
                return res;
            }

            var stringified = Encoding.UTF8.GetString(buffer);
            return JsonParser.Default.Parse(stringified, res.Descriptor);
        }

        public static T FromJson<T>(string json) where T : IMessage, new()
        {
            return JsonParser.Default.Parse<T>(json);
        }
    }
}
