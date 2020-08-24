namespace Pitaya
{
    public interface ISerializerFactory
    {
        IPitayaSerializer CreateJsonSerializer();
        IPitayaSerializer CreateProtobufSerializer(ProtobufSerializer.SerializationFormat format);
    }

    public class DefaultSerializerFactory : ISerializerFactory
    {
        public IPitayaSerializer CreateJsonSerializer()
        {
            return new JsonSerializer();
        }
        
        public IPitayaSerializer CreateProtobufSerializer(ProtobufSerializer.SerializationFormat format)
        {
            return new ProtobufSerializer(format);
        }
        
    }
}