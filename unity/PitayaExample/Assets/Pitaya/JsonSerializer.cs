using Newtonsoft.Json;

namespace Pitaya
{
    public class JsonSerializer : IPitayaSerializer
    {
        JsonSerializerSettings _settings;

        public JsonSerializer()
        {
            _settings = new JsonSerializerSettings();
            _settings.MissingMemberHandling = MissingMemberHandling.Error;
        }
        
        public JsonSerializer(JsonSerializerSettings settings)
        {
            _settings = settings;
        }
        
        public byte[] Encode(object obj)
        {
            string json = JsonConvert.SerializeObject(obj, _settings);
            return System.Text.Encoding.UTF8.GetBytes(json);
        }

        public T Decode<T>(byte[] data)
        {
            string json = System.Text.Encoding.UTF8.GetString(data);
            return (T)JsonConvert.DeserializeObject(json, typeof(T), _settings);
        }
    }
}