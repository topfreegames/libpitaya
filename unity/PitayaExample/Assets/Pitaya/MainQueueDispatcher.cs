using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Pitaya {
    public class MainQueueDispatcher : MonoBehaviour, IPitayaQueueDispatcher
    {
        static MainQueueDispatcher _instance;

        List<Action> _actions;
        List<Action> _actionsCopy;
        readonly object Lock = new object();
        
        public static MainQueueDispatcher Create(bool dontDestroyOnLoad = true)
        {
            if (_instance != null)
                return _instance;
            
            var go = new GameObject (typeof(MainQueueDispatcher).FullName);
            _instance = go.AddComponent<MainQueueDispatcher> ();
            if (dontDestroyOnLoad) DontDestroyOnLoad(_instance.gameObject);
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
                try
                {
                    action();
                }
                catch (Exception e)
                {
                    Debug.LogError("Exception ocurred on dispatcher list: " + e);
                }
            }
            
        }
    }
}