using System;
using System.Collections.Generic;
using System.Threading;

namespace UnityThreading
{
	public class Channel<T> : IDisposable
	{
		private readonly List<T> _buffer = new List<T>();
		private readonly object _setSyncRoot = new object();
		private readonly object _getSyncRoot = new object();
		private readonly object _disposeRoot = new object();
		private ManualResetEvent setEvent = new ManualResetEvent(false);
		private ManualResetEvent getEvent = new ManualResetEvent(true);
		private ManualResetEvent exitEvent = new ManualResetEvent(false);
		private bool disposed;

		private int BufferSize { get; set; }

		protected Channel()
			: this(1)
		{
		}

		private Channel(int bufferSize)
		{
			if (bufferSize < 1)
				throw new ArgumentOutOfRangeException("bufferSize", "Must be greater or equal to 1.");

			BufferSize = bufferSize;
		}

		~Channel()
		{
			Dispose();
		}

		public void Resize(int newBufferSize)
		{
			if (newBufferSize < 1)
				throw new ArgumentOutOfRangeException("newBufferSize", "Must be greater or equal to 1.");

			lock (_setSyncRoot)
			{
				if (disposed)
					return;

				var result = WaitHandle.WaitAny(new WaitHandle[] { exitEvent, getEvent });
				if (result == 0)
					return;

				_buffer.Clear();

				if (newBufferSize != BufferSize)
					BufferSize = newBufferSize;
			}
		}

		public bool Set(T value, int timeoutInMilliseconds = int.MaxValue)
		{
			lock (_setSyncRoot)
			{
				if (disposed)
					return false;
			
				var result = WaitHandle.WaitAny(new WaitHandle[] { exitEvent, getEvent }, timeoutInMilliseconds);
				if (result == WaitHandle.WaitTimeout || result == 0)
					return false;

				_buffer.Add(value);

				if (_buffer.Count != BufferSize) return true;
				setEvent.Set();
				getEvent.Reset();

				return true;
			}
		}

		public T Get(int timeoutInMilliseconds = int.MaxValue, T defaultValue = default(T))
		{
			lock (_getSyncRoot)
			{
				if (disposed)
					return defaultValue;

				var result = WaitHandle.WaitAny(new WaitHandle[] { exitEvent, setEvent }, timeoutInMilliseconds);
				if (result == WaitHandle.WaitTimeout || result == 0)
					return defaultValue;

				var value = _buffer[0];
				_buffer.RemoveAt(0);

				if (_buffer.Count != 0) return value;
				getEvent.Set();
				setEvent.Reset();

				return value;
			}
		}

		public void Close()
		{
			lock (_disposeRoot)
			{
				if (disposed)
					return;

				exitEvent.Set();
			}
		}

		#region IDisposable Members

		public void Dispose()
		{
			if (disposed)
				return;

			lock (_disposeRoot)
			{
				exitEvent.Set();

				lock (_getSyncRoot)
				{
					lock (_setSyncRoot)
					{
						setEvent.Close();
						setEvent = null;

						getEvent.Close();
						getEvent = null;

						exitEvent.Close();
						exitEvent = null;

						disposed = true;
					}
				}
			}
		}

		#endregion
	}

	public class Channel : Channel<object>
	{
	}
}
