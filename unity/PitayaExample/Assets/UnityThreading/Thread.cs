using System;
using System.Collections;
using System.Threading;
using UnityEngine;
using ThreadPriority = System.Threading.ThreadPriority;

namespace UnityThreading
{
	public abstract class ThreadBase : IDisposable
	{
		public static int AvailableProcessors
		{
			get
			{
#if !NO_UNITY
                return SystemInfo.processorCount;
#else
				return Environment.ProcessorCount;
#endif
			}
		}

		private readonly Dispatcher _targetDispatcher;
		private Thread _thread;
        protected readonly ManualResetEvent ExitEvent = new ManualResetEvent(false);

        [ThreadStatic]
        private static ThreadBase _currentThread;
		private readonly string _threadName;

		/// <summary>
		/// Returns the currently ThreadBase instance which is running in this thread.
		/// </summary>
        public static ThreadBase CurrentThread => _currentThread;

		protected ThreadBase(string threadName, bool autoStartThread = true)
			: this(threadName, Dispatcher.CurrentNoThrow, autoStartThread)
        {
        }

		protected ThreadBase(string threadName, Dispatcher targetDispatcher, bool autoStartThread = true)
        {
			_threadName = threadName;
            _targetDispatcher = targetDispatcher;
            if (autoStartThread)
                Start();
        }

		/// <summary>
		/// Returns true if the thread is working.
		/// </summary>
        public bool IsAlive => _thread?.IsAlive ?? false;

		/// <summary>
		/// Returns true if the thread should stop working.
		/// </summary>
		private bool ShouldStop => ExitEvent.InterWaitOne(0);

		/// <summary>
		/// Starts the thread.
		/// </summary>
        public void Start()
        {
            if (_thread != null)
                Abort();

            ExitEvent.Reset();
	        _thread = new Thread(DoInternal)
	        {
		        Name = _threadName,
		        Priority = _priority
	        };

	        _thread.Start();
        }

		/// <summary>
		/// Notifies the thread to stop working.
		/// </summary>
		private void Exit()
        {
            if (_thread != null)
                ExitEvent.Set();
        }

		/// <summary>
		/// Notifies the thread to stop working.
		/// </summary>
        public void Abort()
        {
            Exit();
	        _thread?.Join();
        }

		/// <summary>
		/// Notifies the thread to stop working and waits for completion for the given ammount of time.
		/// When the thread soes not stop after the given timeout the thread will be terminated.
		/// </summary>
		/// <param name="seconds">The time this method will wait until the thread will be terminated.</param>
		private void AbortWaitForSeconds(float seconds)
        {
            Exit();
	        if (_thread == null) return;
	        _thread.Join((int)(seconds * 1000));
	        if (_thread.IsAlive)
		        _thread.Abort();
        }

		/// <summary>
		/// Creates a new Task for the target Dispatcher (default: the main Dispatcher) based upon the given function.
		/// </summary>
		/// <typeparam name="T">The return value of the task.</typeparam>
		/// <param name="function">The function to process at the dispatchers thread.</param>
		/// <returns>The new task.</returns>
		private Task<T> Dispatch<T>(Func<T> function)
        {
            return _targetDispatcher.Dispatch(function);
        }

		/// <summary>
		/// Creates a new Task for the target Dispatcher (default: the main Dispatcher) based upon the given function.
		/// This method will wait for the task completion and returns the return value.
		/// </summary>
		/// <typeparam name="T">The return value of the task.</typeparam>
		/// <param name="function">The function to process at the dispatchers thread.</param>
		/// <returns>The return value of the tasks function.</returns>
        public T DispatchAndWait<T>(Func<T> function)
        {
			var task = Dispatch(function);
            task.Wait();
            return task.Result;
        }

		/// <summary>
		/// Creates a new Task for the target Dispatcher (default: the main Dispatcher) based upon the given function.
		/// This method will wait for the task completion or the timeout and returns the return value.
		/// </summary>
		/// <typeparam name="T">The return value of the task.</typeparam>
		/// <param name="function">The function to process at the dispatchers thread.</param>
		/// <param name="timeOutSeconds">Time in seconds after the waiting process will stop.</param>
		/// <returns>The return value of the tasks function.</returns>
        public T DispatchAndWait<T>(Func<T> function, float timeOutSeconds)
        {
            var task = Dispatch(function);
            task.WaitForSeconds(timeOutSeconds);
            return task.Result;
        }

		/// <summary>
		/// Creates a new Task for the target Dispatcher (default: the main Dispatcher) based upon the given action.
		/// </summary>
		/// <param name="action">The action to process at the dispatchers thread.</param>
		/// <returns>The new task.</returns>
		private Task Dispatch(Action action)
        {
            return _targetDispatcher.Dispatch(action);
        }

		/// <summary>
		/// Creates a new Task for the target Dispatcher (default: the main Dispatcher) based upon the given action.
		/// This method will wait for the task completion.
		/// </summary>
		/// <param name="action">The action to process at the dispatchers thread.</param>
        public void DispatchAndWait(Action action)
        {
            var task = Dispatch(action);
            task.Wait();
        }

		/// <summary>
		/// Creates a new Task for the target Dispatcher (default: the main Dispatcher) based upon the given action.
		/// This method will wait for the task completion or the timeout.
		/// </summary>
		/// <param name="action">The action to process at the dispatchers thread.</param>
		/// <param name="timeOutSeconds">Time in seconds after the waiting process will stop.</param>
		public void DispatchAndWait(Action action, float timeOutSeconds)
        {
			var task = Dispatch(action);
            task.WaitForSeconds(timeOutSeconds);
        }

        /// <summary>
        /// Dispatches the given task to the target Dispatcher (default: the main Dispatcher).
        /// </summary>
        /// <param name="taskBase">The task to process at the dispatchers thread.</param>
        /// <returns>The new task.</returns>
        private Task Dispatch(Task taskBase)
        {
            return _targetDispatcher.Dispatch(taskBase);
        }

        /// <summary>
        /// Dispatches the given task to the target Dispatcher (default: the main Dispatcher).
        /// This method will wait for the task completion.
        /// </summary>
        /// <param name="taskBase">The task to process at the dispatchers thread.</param>
        public void DispatchAndWait(Task taskBase)
        {
            var task = Dispatch(taskBase);
            task.Wait();
        }

        /// <summary>
        /// Dispatches the given task to the target Dispatcher (default: the main Dispatcher).
        /// This method will wait for the task completion or the timeout.
        /// </summary>
        /// <param name="taskBase">The task to process at the dispatchers thread.</param>
        /// <param name="timeOutSeconds">Time in seconds after the waiting process will stop.</param>
        public void DispatchAndWait(Task taskBase, float timeOutSeconds)
        {
            var task = Dispatch(taskBase);
            task.WaitForSeconds(timeOutSeconds);
        }

		private void DoInternal()
        {
            _currentThread = this;

            var enumerator = Do();
            if (enumerator == null)
            {
                return;
            }

			RunEnumerator(enumerator);
        }

		private void RunEnumerator(IEnumerator enumerator)
		{
			do
			{
				var task1 = enumerator.Current as Task;
				if (task1 != null)
				{
					DispatchAndWait(task1);
				}
				else if (enumerator.Current is SwitchTo)
				{
					var switchTo = (SwitchTo)enumerator.Current;
					if (switchTo.Target == SwitchTo.TargetType.Main && CurrentThread != null)
					{
						var task = Task.Create(() =>
							{
								if (enumerator.MoveNext() && !ShouldStop)
									RunEnumerator(enumerator);
							});
						DispatchAndWait(task);
					}
					else if (switchTo.Target == SwitchTo.TargetType.Thread && CurrentThread == null)
					{
						return;
					}
				}
			}
			while (enumerator.MoveNext() && !ShouldStop);
		}

        protected abstract IEnumerator Do();

        #region IDisposable Members

		/// <summary>
		/// Disposes the thread and all resources.
		/// </summary>
        public virtual void Dispose()
        {
            AbortWaitForSeconds(1.0f);
        }

        #endregion

		private ThreadPriority _priority = ThreadPriority.BelowNormal;
		public ThreadPriority Priority
		{
			get { return _priority; }
			set
			{
				_priority = value;
				if (_thread != null)
					_thread.Priority = _priority;
			}
		}
    }

    public sealed class ActionThread : ThreadBase
    {
        private readonly Action<ActionThread> _action;

	    /// <summary>
        /// Creates a new Thread which runs the given action.
        /// </summary>
        /// <param name="action">The action to run.</param>
        /// <param name="autoStartThread">Should the thread start after creation.</param>
        public ActionThread(Action<ActionThread> action, bool autoStartThread = true)
            : base("ActionThread", Dispatcher.Current, false)
        {
            _action = action;
            if (autoStartThread)
                Start();
        }

        protected override IEnumerator Do()
        {
            _action(this);
            return null;
        }
    }

    public sealed class EnumeratableActionThread : ThreadBase
    {
        private readonly Func<ThreadBase, IEnumerator> _enumeratableAction;

	    /// <summary>
	    /// Creates a new Thread which runs the given enumeratable action.
	    /// </summary>
	    /// <param name="enumeratableAction"></param>
	    /// <param name="autoStartThread">Should the thread start after creation.</param>
	    public EnumeratableActionThread(Func<ThreadBase, IEnumerator> enumeratableAction, bool autoStartThread = true)
			: base("EnumeratableActionThread", Dispatcher.Current, false)
        {
            _enumeratableAction = enumeratableAction;
            if (autoStartThread)
                Start();
        }

        protected override IEnumerator Do()
        {
            return _enumeratableAction(this);
        }
    }

	public sealed class TickThread : ThreadBase
	{
		private readonly Action _action;
		private readonly int _tickLengthInMilliseconds;
        private readonly ManualResetEvent _tickEvent = new ManualResetEvent(false);


		/// <summary>
        /// Creates a new Thread which runs the given action.
        /// </summary>
        /// <param name="action">The action to run.</param>
		/// <param name="tickLengthInMilliseconds">Time between ticks.</param>
        /// <param name="autoStartThread">Should the thread start after creation.</param>
        public TickThread(Action action, int tickLengthInMilliseconds, bool autoStartThread = true)
            : base("TickThread", Dispatcher.CurrentNoThrow, false)
        {
			_tickLengthInMilliseconds = tickLengthInMilliseconds;
            _action = action;
            if (autoStartThread)
                Start();
        }

        protected override IEnumerator Do()
        {
            while (!ExitEvent.InterWaitOne(0))
            {
				_action();

                var result = WaitHandle.WaitAny(new WaitHandle[] { ExitEvent, _tickEvent }, _tickLengthInMilliseconds);
				if (result == 0)
                    return null;
			}
            return null;
        }
	}
}
