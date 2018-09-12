namespace Pitaya
{
    public abstract class Serializer<T>
    {
        public abstract T Decode(byte[] data);
    }
}
