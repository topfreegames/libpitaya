using System;
using System.Text;
using Google.Protobuf;
using UnityEngine;

namespace Pitaya
{
    public class ProtobufSerializer : IPitayaSerializer
    {

        public ProtobufSerializer()
        {
        }
        
        public byte[] Encode(object message)
        {
            byte[] buffer = {};
            buffer = ((IMessage) message).ToByteArray();
            return buffer;
        }
        
        public T Decode<T>(byte[] buffer)
        {
            IMessage res = (IMessage) Activator.CreateInstance(typeof(T));
            res.MergeFrom(buffer);
            return (T)res;
        }
    }
}
