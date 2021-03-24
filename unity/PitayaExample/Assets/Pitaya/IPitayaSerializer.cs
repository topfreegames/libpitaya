using System;

namespace Pitaya
{
    public interface IPitayaSerializer
    {
        byte[] Encode(object obj);
        T Decode<T>(byte[] buffer);
    }
}