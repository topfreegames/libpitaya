using System.Collections.Generic;
using System;

namespace Pitaya
{
    public class TypeSubscriber<T>
    {
        private Dictionary<T, Type> _requestSubscriptions;

        public TypeSubscriber()
        {
            _requestSubscriptions = new Dictionary<T, Type>();
        }

        public void Subscribe(T key, Type type)
        {
            if (_requestSubscriptions.ContainsKey(key)) {
                _requestSubscriptions.Remove(key);
            }

            _requestSubscriptions.Add(key, type);
        }

        public Type GetType(T key)
        {
            Type t;
            _requestSubscriptions.TryGetValue(key, out t);
            return t;
        }

        public bool HasType(T key)
        {
            return _requestSubscriptions.ContainsKey(key);
        }

        public void Unsubscribe(T key)
        {
            if (_requestSubscriptions.ContainsKey(key)) {
                _requestSubscriptions.Remove(key);
            }
        }
    }
}
