using Newtonsoft.Json;

namespace Pitaya
{
    public interface ISerializerFactory
    {
        IPitayaSerializer CreateJsonSerializer();
        IPitayaSerializer CreateProtobufSerializer();
    }

    public class SerializerFactory : ISerializerFactory
    {
        private readonly JsonSerializerSettings _settings;

        public SerializerFactory() : this(new JsonSerializerSettings()) {}
        
        public SerializerFactory(JsonSerializerSettings settings)
        {
            _settings = settings;
        }
        
        public IPitayaSerializer CreateJsonSerializer()
        {
            return new JsonSerializer(_settings);
        }
        
        public IPitayaSerializer CreateProtobufSerializer()
        {
            return new ProtobufSerializer();
        }
        
    }
}