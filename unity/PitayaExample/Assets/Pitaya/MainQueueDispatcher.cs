using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Pitaya {
    public class MainQueueDispatcher : MonoBehaviour
    {
        private static MainQueueDispatcher _instance;
        public static void Dispatch (Action action) { _instance._Dispatch (action); }
        public static void DispatchAfter (float seconds, Action action) { _instance._DispatchAfter (seconds, action); }

        private static void Create()
        {
            if (_instance != null)
                return;
            
            var go = new GameObject (typeof(MainQueueDispatcher).FullName);
            _instance = go.AddComponent<MainQueueDispatcher> ();
            DontDestroyOnLoad(_instance.gameObject);
        }

        private void _DispatchAfter(float seconds, Action action)
        {
            StartCoroutine(DispatchAfterProcess(seconds, action));
        }

        private static IEnumerator DispatchAfterProcess(float seconds, Action action)
        {
            yield return new WaitForSeconds(seconds);
            if (action != null ) action.Invoke();
        }

        [RuntimeInitializeOnLoadMethod (RuntimeInitializeLoadType.BeforeSceneLoad)]
        private static void Initialize () {
            Create ();
        }

        private List<Action> _actions;
        private List<Action> _actionsCopy;
        
        private static readonly object Lock = new object();
        
        

        private void Awake () {
            lock (Lock)
            {
                _actions = new List<Action>();
                _actionsCopy = new List<Action>();
            }
        }

    	private void _Dispatch (Action action) {
	        lock (Lock)
	        {
	            _actions.Add (action);
	        }
        }

        private void Update () {
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