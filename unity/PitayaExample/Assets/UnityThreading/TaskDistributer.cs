using System;
using System.Collections;
using System.Linq;
using System.Threading;

namespace UnityThreading
{
    public class TaskDistributor : DispatcherBase
	{
        private TaskWorker[] workerThreads;

        internal WaitHandle NewDataWaitHandle => DataEvent;

		private static TaskDistributor mainTaskDistributor;

		/// <summary>
		/// Returns the first created TaskDistributor instance. When no instance has been created an exception will be thrown.
		/// </summary>
		public static TaskDistributor Main
		{
			get
			{
				if (mainTaskDistributor == null)
					throw new InvalidOperationException("No default TaskDistributor found, please create a new TaskDistributor instance before calling this property.");

				return mainTaskDistributor;
			}
		}

		/// <summary>
		/// Returns the first created TaskDistributor instance.
		/// </summary>
		public static TaskDistributor MainNoThrow
		{
			get
			{
				return mainTaskDistributor;
			}
		}

		public override int TaskCount
        {
            get
            {
                var count = base.TaskCount;
                lock (workerThreads)
                {
	                count += workerThreads.Sum(t => t.Dispatcher.TaskCount);
                }
                return count;
            }
        }

		/// <inheritdoc />
		/// <summary>
		/// Creates a new instance of the TaskDistributor.
		/// The task distributor will auto start his worker threads.
		/// </summary>
		/// <param name="name"></param>
		/// <param name="workerThreadCount">The number of worker threads, a value below one will create ProcessorCount x2 worker threads.</param>
		public TaskDistributor(string name, int workerThreadCount = 0)
			: this(name, workerThreadCount, true)
		{
		}

		/// <summary>
		/// Creates a new instance of the TaskDistributor.
		/// </summary>
		/// <param name="name"></param>
		/// <param name="workerThreadCount">The number of worker threads, a value below one will create ProcessorCount x2 worker threads.</param>
		/// <param name="autoStart">Should the instance auto start the worker threads.</param>
		private TaskDistributor(string name, int workerThreadCount, bool autoStart)
		{
			this._name = name;
            if (workerThreadCount <= 0)
				workerThreadCount = ThreadBase.AvailableProcessors * 2;

			workerThreads = new TaskWorker[workerThreadCount];
			lock (workerThreads)
			{
				for (var i = 0; i < workerThreadCount; ++i)
					workerThreads[i] = new TaskWorker(name, this);
			}

			if (mainTaskDistributor == null)
				mainTaskDistributor = this;

			if (autoStart)
				Start();
		}

		/// <summary>
		/// Starts the TaskDistributor if its not currently running.
		/// </summary>
		private void Start()
		{
			lock (workerThreads)
			{
				foreach (var t in workerThreads)
				{
					if (!t.IsAlive)
					{
						t.Start();
					}
				}
			}
		}

		private void SpawnAdditionalWorkerThread()
        {
            lock (workerThreads)
            {
                Array.Resize(ref workerThreads, workerThreads.Length + 1);
	            workerThreads[workerThreads.Length - 1] = new TaskWorker(_name, this) { Priority = _priority };
	            workerThreads[workerThreads.Length - 1].Start();
            }
        }

        /// <summary>
        /// Amount of additional spawnable worker threads.
        /// </summary>
        private int _maxAdditionalWorkerThreads;

		private readonly string _name;

        internal void FillTasks(Dispatcher target)
        {
			target.AddTasks(IsolateTasks(1));
        }

		protected override void CheckAccessLimitation()
		{
			if (_maxAdditionalWorkerThreads > 0 || !AllowAccessLimitationChecks)
                return;

			if (ThreadBase.CurrentThread != null &&
				ThreadBase.CurrentThread is TaskWorker &&
				((TaskWorker)ThreadBase.CurrentThread).TaskDistributor == this)
			{
				throw new InvalidOperationException("Access to TaskDistributor prohibited when called from inside a TaskDistributor thread. Dont dispatch new Tasks through the same TaskDistributor. If you want to distribute new tasks create a new TaskDistributor and use the new created instance. Remember to dispose the new instance to prevent thread spamming.");
			}
		}

        internal override void TasksAdded()
        {
			if (_maxAdditionalWorkerThreads > 0 &&
				(workerThreads.All(worker => worker.Dispatcher.TaskCount > 0 || worker.IsWorking) || TaskList.Count > workerThreads.Length))
            {
				Interlocked.Decrement(ref _maxAdditionalWorkerThreads);
                SpawnAdditionalWorkerThread();
            }

			base.TasksAdded();
        }

        #region IDisposable Members

		private bool isDisposed;
		/// <summary>
		/// Disposes all TaskDistributor, worker threads, resources and remaining tasks.
		/// </summary>
        public override void Dispose()
        {
			if (isDisposed)
				return;

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

			lock (workerThreads)
			{
				for (var i = 0; i < workerThreads.Length; ++i)
					workerThreads[i].Dispose();
				workerThreads = new TaskWorker[0];
			}

			DataEvent.Close();
			DataEvent = null;

			if (mainTaskDistributor == this)
				mainTaskDistributor = null;

			isDisposed = true;
        }

        #endregion

		private ThreadPriority _priority = ThreadPriority.BelowNormal;
		public ThreadPriority Priority
		{
			get { return _priority; }
			set
			{
				_priority = value;
				foreach (var worker in workerThreads)
					worker.Priority = value;
			}
		}
	}

    internal sealed class TaskWorker : ThreadBase
    {
		public Dispatcher Dispatcher;
		public TaskDistributor TaskDistributor { get; private set; }

		public bool IsWorking
		{
			get
			{
				return Dispatcher.IsWorking;
			}
		}

		public TaskWorker(string name, TaskDistributor taskDistributor)
            : base(name, false)
        {
			TaskDistributor = taskDistributor;
			Dispatcher = new Dispatcher(false);
		}

        protected override IEnumerator Do()
        {
            while (!ExitEvent.InterWaitOne(0))
            {
	            if (Dispatcher.ProcessNextTask()) continue;
	            TaskDistributor.FillTasks(Dispatcher);

	            if (Dispatcher.TaskCount != 0) continue;
	            var result = WaitHandle.WaitAny(new[] { ExitEvent, TaskDistributor.NewDataWaitHandle });
	            if (result == 0)
		            return null;
	            TaskDistributor.FillTasks(Dispatcher);
            }
            return null;
        }

        public override void Dispose()
        {
            base.Dispose();
	        Dispatcher?.Dispose();
	        Dispatcher = null;
        }
	}
}

