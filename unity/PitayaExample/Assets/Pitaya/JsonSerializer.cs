using System.Text;


namespace Pitaya
{
    public static class JsonSerializer
    {
        public static byte[] Encode(string obj)
        {
            return obj != null ? Encoding.UTF8.GetBytes(obj) : null;
        }

        public static string Decode(byte[] data)
        {
            return Encoding.UTF8.GetString(data);
        }
    }
}
