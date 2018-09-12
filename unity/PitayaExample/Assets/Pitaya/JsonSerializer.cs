using System.Text;
using SimpleJson;

namespace Pitaya
{
    public static class JsonSerializer
    {
        public static byte[] Encode(object obj)
        {
            if (!(obj is JsonObject)) {
                return null;
            }

            var stringified = ((JsonObject)obj).ToString();
            return Encoding.UTF8.GetBytes(stringified);
        }

        public static JsonObject Decode(byte[] data)
        {
            var stringified = Encoding.UTF8.GetString(data);
            return (JsonObject)SimpleJson.SimpleJson.DeserializeObject(stringified);
        }
    }
}
