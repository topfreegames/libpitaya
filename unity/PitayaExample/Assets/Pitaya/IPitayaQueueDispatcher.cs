using System;

namespace Pitaya
{
    public interface IPitayaQueueDispatcher
    {
        void Dispatch(Action action);
    }

    public class NullPitayaQueueDispatcher : IPitayaQueueDispatcher
    {
        public void Dispatch(Action action)
        {
            action();
        }
    }
}