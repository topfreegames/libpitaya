using System;
using System.Collections.Generic;

namespace Pitaya
{
    public class EventManager : IDisposable
    {
        private readonly Dictionary<uint, Action<byte[]>> _callBackMap;
       
        private readonly Dictionary<uint, Action<PitayaError>> _errorCallBackMap;
        private readonly Dictionary<string, List<Action<byte[]>>> _eventMap;

        public EventManager()
        {
            _callBackMap = new Dictionary<uint, Action<byte[]>>();
            _eventMap = new Dictionary<string, List<Action<byte[]>>>();
            _errorCallBackMap = new Dictionary<uint, Action<PitayaError>>();
        }

        //Adds callback to callBackMap by id.
        public void AddCallBack(uint id, Action<byte[]> callback, Action<PitayaError> errorCallBack)
        {
            if (id > 0 && callback != null)
            {
                _callBackMap.Add(id, callback);
            }
            if (id > 0 && errorCallBack != null)
            {
                _errorCallBackMap.Add(id, errorCallBack);
            }
        }

        public void InvokeCallBack(uint id, byte[] data)
        {
            Action<byte[]> action = null;
            var foundAction = _callBackMap.TryGetValue(id, out action);
            
            ClearCallbacks(id);
            
            if (foundAction) action.Invoke(data);
        }

        public void InvokeErrorCallBack(uint id, PitayaError error)
        {
            Action<PitayaError> action = null;
            var foundAction = _errorCallBackMap.TryGetValue(id, out action);
            
            ClearCallbacks(id);
            
            if (foundAction) action.Invoke(error);
        }

        private void ClearCallbacks(uint id)
        {
            _callBackMap.Remove(id);
            _errorCallBackMap.Remove(id);
        }

        public void ClearAllCallbacks()
        {
            _callBackMap.Clear();
            _errorCallBackMap.Clear();
        }

        // Adds the event to eventMap by name.
        public void AddOnRouteEvent(string routeName, Action<byte[]> callback)
        {
            List<Action<byte[]>> list;
            if (_eventMap.TryGetValue(routeName, out list))
            {
                list.Add(callback);
            }
            else
            {
                list = new List<Action<byte[]>> { callback };
                _eventMap.Add(routeName, list);
            }
        }

        public void RemoveOnRouteEvent(string routeName) {
            _eventMap.Remove(routeName);
        }

        public void RemoveAllOnRouteEvents()
        {
            _eventMap.Clear();
        }

        /// <summary>
        /// If the event exists,invoke the event when server return messge.
        /// </summary>
        /// <returns></returns>
        ///
        public void InvokeOnEvent(string route, byte[] msg)
        {
            if (!_eventMap.ContainsKey(route)) return;

            var list = _eventMap[route];
            foreach (var action in list) action.Invoke(msg);
        }

        // Dispose() calls Dispose(true)
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        // The bulk of the clean-up code is implemented in Dispose(bool)
        // ReSharper disable once UnusedParameter.Local
        private void Dispose(bool disposing)
        {
            var errorCallBackMap = new Dictionary<uint, Action<PitayaError>>(_errorCallBackMap);
            foreach (var callback in errorCallBackMap)
                callback.Value.Invoke(new PitayaError(PitayaConstants.PitayaInternalError, "pitaya exited"));

            _callBackMap.Clear();
            _eventMap.Clear();
            _errorCallBackMap.Clear();
        }
    }
}
