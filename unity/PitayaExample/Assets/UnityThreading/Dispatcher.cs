using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

namespace UnityThreading
{
    public abstract class DispatcherBase : IDisposable
    {
	    private int _lockCount;
		protected readonly object TaskListSyncRoot = new object();
	    private readonly Queue<Task> _delayedTaskList = new Queue<Task>();
	    protected Queue<Task> TaskList = new Queue<Task>();
        protected ManualResetEvent DataEvent = new ManualResetEvent(false);

	    public bool IsWorking => DataEvent.InterWaitOne(0);

	    protected bool AllowAccessLimitationChecks;

        /// <summary>
        /// Set the task reordering system
        /// </summary>
        protected TaskSortingSystem TaskSortingSystem;

		/// <summary>
		/// Returns the currently existing task count. Early aborted tasks will count too.
		/// </summary>
        public virtual int TaskCount
        {
            get
            {
				lock (TaskListSyncRoot)
                    return TaskList.Count;
            }
        }

        public void Lock()
        {
            lock (TaskListSyncRoot)
            {
                _lockCount++;
            }
        }

        public void Unlock()
        {
            lock (TaskListSyncRoot)
            {
                _lockCount--;
	            if (_lockCount != 0 || _delayedTaskList.Count <= 0) return;

	            while (_delayedTaskList.Count > 0)
		            TaskList.Enqueue(_delayedTaskList.Dequeue());

	            if (TaskSortingSystem == TaskSortingSystem.ReorderWhenAdded ||
	                TaskSortingSystem == TaskSortingSystem.ReorderWhenExecuted)
		            ReorderTasks();

	            TasksAdded();
            }
        }

		/// <summary>
		/// Creates a new Task based upon the given action.
		/// </summary>
		/// <typeparam name="T">The return value of the task.</typeparam>
		/// <param name="function">The function to process at the dispatchers thread.</param>
		/// <returns>The new task.</returns>
		public Task<T> Dispatch<T>(Func<T> function)
        {
			CheckAccessLimitation();

            var task = new Task<T>(function);
            AddTask(task);
            return task;
        }

		/// <summary>
		/// Creates a new Task based upon the given action.
		/// </summary>
		/// <param name="action">The action to process at the dispatchers thread.</param>
		/// <returns>The new task.</returns>
        public Task Dispatch(Action action)
        {
			CheckAccessLimitation();

            var task = Task.Create(action);
            AddTask(task);
            return task;
        }

	    /// <summary>
	    /// Dispatches a given Task.
	    /// </summary>
	    /// <returns>The new task.</returns>
	    public Task Dispatch(Task task)
        {
            CheckAccessLimitation();

            AddTask(task);
            return task;
        }

	    protected virtual void AddTask(Task task)
        {
            lock (TaskListSyncRoot)
            {
                if (_lockCount > 0)
                {
                    _delayedTaskList.Enqueue(task);
                    return;
                }

                TaskList.Enqueue(task);
                
                if (TaskSortingSystem == TaskSortingSystem.ReorderWhenAdded ||
                    TaskSortingSystem == TaskSortingSystem.ReorderWhenExecuted)
					 ReorderTasks();
            }
			TasksAdded();
        }

        internal void AddTasks(IEnumerable<Task> tasks)
        {
            lock (TaskListSyncRoot)
            {
                if (_lockCount > 0)
                {
                    foreach (var task in tasks)
                        _delayedTaskList.Enqueue(task);
                    return;
                }

                foreach (var task in tasks)
                    TaskList.Enqueue(task);

                if (TaskSortingSystem == TaskSortingSystem.ReorderWhenAdded || TaskSortingSystem == TaskSortingSystem.ReorderWhenExecuted)
					 ReorderTasks();
            }
			TasksAdded();
        }

        internal virtual void TasksAdded()
        {
            DataEvent.Set();
        }

		protected void ReorderTasks()
        {
			TaskList = new Queue<Task>(TaskList.OrderBy(t => t.Priority));
        }

		internal IEnumerable<Task> SplitTasks(int divisor)
        {
			if (divisor == 0)
				divisor = 2;
			var count = TaskCount / divisor;
            return IsolateTasks(count);
        }

        internal IEnumerable<Task> IsolateTasks(int count)
        {
			var newTasks = new Queue<Task>();

			if (count == 0)
				count = TaskList.Count;

            lock (TaskListSyncRoot)
            {
				for (var i = 0; i < count && i < TaskList.Count; ++i)
					newTasks.Enqueue(TaskList.Dequeue());
                
				//if (TaskSortingSystem == TaskSortingSystem.ReorderWhenExecuted)
				//    taskList = ReorderTasks(taskList);

				if (TaskCount == 0)
					DataEvent.Reset();
            }

			return newTasks;
        }

		protected abstract void CheckAccessLimitation();

        #region IDisposable Members

        public virtual void Dispose()
        {
			while (true)
			{
				Task currentTask;
                lock (TaskListSyncRoot)
				{
                    if (TaskList.Count != 0)
						currentTask = TaskList.Dequeue();
                    else
                        break;
				}
				currentTask.Dispose();
			}

			DataEvent.Close();
			DataEvent = null;
        }

        #endregion
    }

	public class NullDispatcher : DispatcherBase
	{
		public static NullDispatcher Null = new NullDispatcher();
		protected override void CheckAccessLimitation()
		{
		}

		protected override void AddTask(Task task)
		{
			task.DoInternal();
		}
	}

    public class Dispatcher : DispatcherBase
    {
        [ThreadStatic]
        private static Task _currentTask;

        [ThreadStatic]
        private static Dispatcher _currentDispatcher;

	    private static Dispatcher _mainDispatcher;

		/// <summary>
		/// Returns the task which is currently being processed. Use this only inside a task operation.
		/// </summary>
		private static Task CurrentTask
        {
            get
            {
                if (_currentTask == null)
                    throw new InvalidOperationException("No task is currently running.");

                return _currentTask;
            }
        }

		/// <summary>
		/// Returns the Dispatcher instance of the current thread. When no instance has been created an exception will be thrown.
		/// </summary>
        /// 
        public static Dispatcher Current
        {
            get
			{
				if (_currentDispatcher == null)
					throw new InvalidOperationException("No Dispatcher found for the current thread, please create a new Dispatcher instance before calling this property.");
				return _currentDispatcher; 
			}
            set
            {
	            _currentDispatcher?.Dispose();
	            _currentDispatcher = value;
            }
        }

        /// <summary>
        /// Returns the Dispatcher instance of the current thread.
        /// </summary>
        /// 
        public static Dispatcher CurrentNoThrow => _currentDispatcher;

	    /// <summary>
		/// Returns the first created Dispatcher instance, in most cases this will be the Dispatcher for the main thread. When no instance has been created an exception will be thrown.
		/// </summary>
        public static Dispatcher Main
        {
            get
            {
				if (_mainDispatcher == null)
					throw new InvalidOperationException("No Dispatcher found for the main thread, please create a new Dispatcher instance before calling this property.");

                return _mainDispatcher;
            }
        }

        /// <summary>
        /// Returns the first created Dispatcher instance.
        /// </summary>
        public static Dispatcher MainNoThrow
        {
            get
            {
                return _mainDispatcher;
            }
        }

		/// <summary>
		/// Creates a new function based upon an other function which will handle exceptions. Use this to wrap safe functions for tasks.
		/// </summary>
		/// <typeparam name="T">The return type of the function.</typeparam>
		/// <param name="function">The orignal function.</param>
		/// <returns>The safe function.</returns>
		public static Func<T> CreateSafeFunction<T>(Func<T> function)
		{
			return () =>
				{
					try
					{
						return function();
					}
					catch
					{
						CurrentTask.Abort();
						return default(T);
					}
				};
		}

	    /// <summary>
	    /// Creates a new action based upon an other action which will handle exceptions. Use this to wrap safe action for tasks.
	    /// </summary>
	    /// <returns>The safe action.</returns>
	    public static Action CreateSafeAction(Action action)
		{
			return () =>
			{
				try
				{
					action();
				}
				catch
				{
					CurrentTask.Abort();
				}
			};
		}

		/// <inheritdoc />
		/// <summary>
		/// Creates a Dispatcher, if a Dispatcher has been created in the current thread an exception will be thrown.
		/// </summary>
		public Dispatcher()
			: this(true)
		{
		}

        /// <summary>
        /// Creates a Dispatcher, if a Dispatcher has been created when setThreadDefaults is set to true in the current thread an exception will be thrown.
        /// </summary>
        /// <param name="setThreadDefaults">If set to true the new dispatcher will be set as threads default dispatcher.</param>
		public Dispatcher(bool setThreadDefaults)
        {
			if (!setThreadDefaults)
				return;

            if (_currentDispatcher != null)
				throw new InvalidOperationException("Only one Dispatcher instance allowed per thread.");

			_currentDispatcher = this;

            if (_mainDispatcher == null)
                _mainDispatcher = this;
        }

		/// <summary>
		/// Processes all remaining tasks. Call this periodically to allow the Dispatcher to handle dispatched tasks.
        /// Only call this inside the thread you want the tasks to process to be processed.
		/// </summary>
        public void ProcessTasks()
        {
			if (DataEvent.InterWaitOne(0))
				ProcessTasksInternal();
        }

		/// <summary>
		/// Processes all remaining tasks and returns true when something has been processed and false otherwise.
		/// This method will block until th exitHandle has been set or tasks should be processed.
        /// Only call this inside the thread you want the tasks to process to be processed.
		/// </summary>
		/// <param name="exitHandle">The handle to indicate an early abort of the wait process.</param>
		/// <returns>False when the exitHandle has been set, true otherwise.</returns>
        public bool ProcessTasks(WaitHandle exitHandle)
        {
            var result = WaitHandle.WaitAny(new[] { exitHandle, DataEvent });
			if (result == 0)
                return false;
            ProcessTasksInternal();
            return true;
        }

		/// <summary>
		/// Processed the next available task.
        /// Only call this inside the thread you want the tasks to process to be processed.
		/// </summary>
		/// <returns>True when a task to process has been processed, false otherwise.</returns>
        public bool ProcessNextTask()
        {
			Task task;
			lock (TaskListSyncRoot)
			{
				if (TaskList.Count == 0)
					return false;
				task = TaskList.Dequeue();
			}

			ProcessSingleTask(task);

            if (TaskCount == 0)
                DataEvent.Reset();

            return true;
        }

		/// <summary>
		/// Processes the next available tasks and returns true when it has been processed and false otherwise.
		/// This method will block until th exitHandle has been set or a task should be processed.
        /// Only call this inside the thread you want the tasks to process to be processed.
		/// </summary>
		/// <param name="exitHandle">The handle to indicate an early abort of the wait process.</param>
		/// <returns>False when the exitHandle has been set, true otherwise.</returns>
        public bool ProcessNextTask(WaitHandle exitHandle)
        {
            var result = WaitHandle.WaitAny(new[] { exitHandle, DataEvent });
			if (result == 0)
                return false;

			Task task;
			lock (TaskListSyncRoot)
			{
				if (TaskList.Count == 0)
					return false;
				task = TaskList.Dequeue();
			}

			ProcessSingleTask(task);
			if (TaskCount == 0)
				DataEvent.Reset();

            return true;
        }

        private void ProcessTasksInternal()
        {
			List<Task> tmpCopy;
            lock (TaskListSyncRoot)
            {
				tmpCopy = new List<Task>(TaskList);
				TaskList.Clear();
			}

			while (tmpCopy.Count != 0)
			{
				var task = tmpCopy[0];
				tmpCopy.RemoveAt(0);
				ProcessSingleTask(task);
			}

            if (TaskCount == 0)
                DataEvent.Reset();
		}

        private void ProcessSingleTask(Task task)
        {
            RunTask(task);

	        if (TaskSortingSystem != TaskSortingSystem.ReorderWhenExecuted) return;

	        lock (TaskListSyncRoot)
		        ReorderTasks();
        }

	    private static void RunTask(Task task)
		{
			var oldTask = _currentTask;
			_currentTask = task;
			_currentTask.DoInternal();
			_currentTask = oldTask;
		}

		protected override void CheckAccessLimitation()
		{
			if (AllowAccessLimitationChecks && _currentDispatcher == this)
				throw new InvalidOperationException("Dispatching a Task with the Dispatcher associated to the current thread is prohibited. You can run these Tasks without the need of a Dispatcher.");
		}

        #region IDisposable Members

		/// <summary>
		/// Disposes all dispatcher resources and remaining tasks.
		/// </summary>
        public override void Dispose()
        {
			while (true)
			{
				lock (TaskListSyncRoot)
				{
                    if (TaskList.Count != 0)
						_currentTask = TaskList.Dequeue();
                    else
                        break;
				}
				_currentTask.Dispose();
			}

			DataEvent.Close();
			DataEvent = null;

			if (_currentDispatcher == this)
				_currentDispatcher = null;
			if (_mainDispatcher == this)
				_mainDispatcher = null;
        }

        #endregion
    }
}
