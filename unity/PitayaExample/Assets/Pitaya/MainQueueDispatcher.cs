using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Pitaya {
    public static class MainQueueDispatcherFactory
    {
        public static MainQueueDispatcher Create(bool dontDestroyOnLoad = true, bool shouldThrowExceptions = false)
        {
            var go = new GameObject (typeof(MainQueueDispatcher).FullName);
            var instance = go.AddComponent<MainQueueDispatcher> ();
            instance.ShouldThrowExceptions = shouldThrowExceptions;
            if (dontDestroyOnLoad) GameObject.DontDestroyOnLoad(instance.gameObject);
            return instance;
        }
    }

    public class MainQueueDispatcher : MonoBehaviour, IPitayaQueueDispatcher
    {
        public static string ExceptionErrorMessage = "Exception ocurred on dispatcher list";

        static MainQueueDispatcher _instance;

        List<Action> _actions;
        List<Action> _actionsCopy;
        readonly object Lock = new object();

        public bool ShouldThrowExceptions { get; set; }

        public static MainQueueDispatcher Create(bool dontDestroyOnLoad = true, bool shouldThrowExceptions = false)
        {
            if (_instance != null)
                return _instance;

            _instance = MainQueueDispatcherFactory.Create(dontDestroyOnLoad, shouldThrowExceptions);
            return _instance;
        }

        public void Dispatch (Action action) {
            lock (Lock)
            {
                _actions.Add (action);
            }
        }

        void Awake () {
            lock (Lock)
            {
                _actions = new List<Action>();
                _actionsCopy = new List<Action>();
            }
        }

        void Update () {
            lock (Lock)
            {
                _actionsCopy.Clear();
                _actionsCopy.AddRange(_actions);
                _actions.Clear();
            }

            foreach (var action in _actionsCopy)
            {
                if (ShouldThrowExceptions) action();
                else SafeInvoke(action);
            }

        }

        void SafeInvoke(Action action) {
            try
            {
                action();
            }
            catch (Exception e)
            {
                Debug.LogError($"{ExceptionErrorMessage}: " + e);
            }
        }
    }
}
