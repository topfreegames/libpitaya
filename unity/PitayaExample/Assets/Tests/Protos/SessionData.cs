using System;
using System.Collections.Generic;
using System.Security.Cryptography;

namespace Pitaya.Json
{
    [Serializable]
    public class JsonSessionData
    {
        public Dictionary<string, string> data;
        public string name;
    }
}