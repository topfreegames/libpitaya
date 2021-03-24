using System;
using System.Text;


namespace Pitaya
{
    public class LegacyJsonSerializer : IPitayaSerializer
    {
        public byte[] Encode(object obj)
        {
            return obj != null ? Encoding.UTF8.GetBytes((string)obj) : null;
        }

        public T Decode<T>(byte[] data)
        {
            return (T)Convert.ChangeType(Encoding.UTF8.GetString(data), typeof(T));
        }
    }
}
