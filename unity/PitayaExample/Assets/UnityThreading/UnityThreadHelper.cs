using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
#if !NO_UNITY

#endif

#if !NO_UNITY
namespace UnityThreading
{
    [ExecuteInEditMode]
    public class UnityThreadHelper : MonoBehaviour
#else
public class UnityThreadHelper
#endif
    {
        private static UnityThreadHelper _instance;
        private static readonly object SyncRoot = new object();

        public static void EnsureHelper()
        {
            lock (SyncRoot)
            {
#if !NO_UNITY
                if (_instance != null) return;
                _instance = FindObjectOfType(typeof(UnityThreadHelper)) as UnityThreadHelper;
                if (_instance != null) return;
                var go = new GameObject("[UnityThreadHelper]")
                {
                    hideFlags = HideFlags.NotEditable | HideFlags.HideInHierarchy | HideFlags.HideInInspector
                };
                _instance = go.AddComponent<UnityThreadHelper>();
                DontDestroyOnLoad(_instance);
                _instance.EnsureHelperInstance();
#else
		    if (null == instance)
		    {
			    instance = new UnityThreadHelper();
			    instance.EnsureHelperInstance();
		    }
#endif
            }
        }

        private static UnityThreadHelper Instance
        {
            get
            {
                EnsureHelper();
                return _instance;
            }
        }

        public static bool IsValid => _instance != null;

        /// <summary>
        /// Returns the GUI/Main Dispatcher.
        /// </summary>
        public static Dispatcher Dispatcher => Instance.CurrentDispatcher;

        /// <summary>
        /// Returns the TaskDistributor.
        /// </summary>
        public static TaskDistributor TaskDistributor => Instance.CurrentTaskDistributor;

        private Dispatcher _dispatcher;

        private Dispatcher CurrentDispatcher => _dispatcher;

        private TaskDistributor _taskDistributor;
        private TaskDistributor CurrentTaskDistributor => _taskDistributor;

        private void EnsureHelperInstance()
        {
            _dispatcher = Dispatcher.MainNoThrow ?? new Dispatcher();
            _taskDistributor = TaskDistributor.MainNoThrow ?? new TaskDistributor("TaskDistributor");
        }

        /// <summary>
        /// Creates new thread which runs the given action. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The action which the new thread should run.</param>
        /// <param name="autoStartThread">True when the thread should start immediately after creation.</param>
        /// <returns>The instance of the created thread class.</returns>
        private static ActionThread CreateThread(System.Action<ActionThread> action, bool autoStartThread)
        {
            Instance.EnsureHelperInstance();

            System.Action<ActionThread> actionWrapper = currentThread =>
            {
                try
                {
                    action(currentThread);
                }
                catch (System.Exception ex)
                {
                    Debug.LogError(ex);
                }
            };
            var thread = new ActionThread(actionWrapper, autoStartThread);
            Instance.RegisterThread(thread);
            return thread;
        }

        /// <summary>
        /// Creates new thread which runs the given action and starts it after creation. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The action which the new thread should run.</param>
        /// <returns>The instance of the created thread class.</returns>
        public static ActionThread CreateThread(System.Action<ActionThread> action)
        {
            return CreateThread(action, true);
        }

        /// <summary>
        /// Creates new thread which runs the given action. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The action which the new thread should run.</param>
        /// <param name="autoStartThread">True when the thread should start immediately after creation.</param>
        /// <returns>The instance of the created thread class.</returns>
        public static ActionThread CreateThread(System.Action action, bool autoStartThread)
        {
            return CreateThread((thread) => action(), autoStartThread);
        }

        /// <summary>
        /// Creates new thread which runs the given action and starts it after creation. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The action which the new thread should run.</param>
        /// <returns>The instance of the created thread class.</returns>
        public static ActionThread CreateThread(System.Action action)
        {
            return CreateThread((thread) => action(), true);
        }

        #region Enumeratable

        /// <summary>
        /// Creates new thread which runs the given action. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The enumeratable action which the new thread should run.</param>
        /// <param name="autoStartThread">True when the thread should start immediately after creation.</param>
        /// <returns>The instance of the created thread class.</returns>
        public static ThreadBase CreateThread(System.Func<ThreadBase, IEnumerator> action, bool autoStartThread)
        {
            Instance.EnsureHelperInstance();

            var thread = new EnumeratableActionThread(action, autoStartThread);
            Instance.RegisterThread(thread);
            return thread;
        }

        /// <summary>
        /// Creates new thread which runs the given action and starts it after creation. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The enumeratable action which the new thread should run.</param>
        /// <returns>The instance of the created thread class.</returns>
        public static ThreadBase CreateThread(System.Func<ThreadBase, IEnumerator> action)
        {
            return CreateThread(action, true);
        }

        /// <summary>
        /// Creates new thread which runs the given action. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The enumeratable action which the new thread should run.</param>
        /// <param name="autoStartThread">True when the thread should start immediately after creation.</param>
        /// <returns>The instance of the created thread class.</returns>
        public static ThreadBase CreateThread(System.Func<IEnumerator> action, bool autoStartThread)
        {
            System.Func<ThreadBase, IEnumerator> wrappedAction = (thread) => { return action(); };
            return CreateThread(wrappedAction, autoStartThread);
        }

        /// <summary>
        /// Creates new thread which runs the given action and starts it after creation. The given action will be wrapped so that any exception will be catched and logged.
        /// </summary>
        /// <param name="action">The action which the new thread should run.</param>
        /// <returns>The instance of the created thread class.</returns>
        public static ThreadBase CreateThread(System.Func<IEnumerator> action)
        {
            System.Func<ThreadBase, IEnumerator> wrappedAction = (thread) => { return action(); };
            return CreateThread(wrappedAction, true);
        }

        #endregion

        private readonly List<ThreadBase> _registeredThreads = new List<ThreadBase>();
        
        private void RegisterThread(ThreadBase thread)
        {
            if (_registeredThreads.Contains(thread))
            {
                return;
            }

            _registeredThreads.Add(thread);
        }

#if !NO_UNITY

        private void OnDestroy()
        {
            foreach (var thread in _registeredThreads)
                thread.Dispose();

            _dispatcher?.Dispose();
            _dispatcher = null;

            _taskDistributor?.Dispose();
            _taskDistributor = null;

            if (_instance == this)
                _instance = null;
        }

        private void Update()
        {
            _dispatcher?.ProcessTasks();

            var finishedThreads = _registeredThreads.Where(thread => !thread.IsAlive).ToArray();
            foreach (var finishedThread in finishedThreads)
            {
                finishedThread.Dispose();
                _registeredThreads.Remove(finishedThread);
            }
        }
#endif
    }
}
