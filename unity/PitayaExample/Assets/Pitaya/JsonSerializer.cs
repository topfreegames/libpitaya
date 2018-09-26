using System.Text;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;


namespace Pitaya
{
    public static class JsonSerializer
    {
        public static byte[] Encode(object obj)
        {
            if (!(obj is JObject)) {
                return null;
            }

            var stringified = JsonConvert.SerializeObject(obj);
            return Encoding.UTF8.GetBytes(stringified);
        }

        public static JObject Decode(byte[] data)
        {
            var stringified = Encoding.UTF8.GetString(data);
            return JObject.Parse(stringified);
        }
    }
}
