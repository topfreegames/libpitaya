using System.Collections;
using System.Text.RegularExpressions;
using System.Threading;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Pitaya.Tests
{
    [TestFixture, Category("integration")]
    public class MainQueueDispatcherTest
    {
        const string expectedException = "expected exception";

        [UnityTest]
        public IEnumerator ShouldThrow()
        {
            var dispatcher = MainQueueDispatcherFactory.Create(false, shouldThrowExceptions: true);
            dispatcher.Dispatch(() => {
                throw new System.Exception(expectedException);
            });
            yield return null;
            GameObject.Destroy(dispatcher.gameObject);

            LogAssert.Expect(LogType.Exception, new Regex($".*{expectedException}"));
        }

        [UnityTest]
        public IEnumerator ShouldNotThrow()
        {
            var dispatcher = MainQueueDispatcherFactory.Create(false);
            dispatcher.Dispatch(() => {
                throw new System.Exception(expectedException);
            });
            yield return null;
            GameObject.Destroy(dispatcher.gameObject);

            LogAssert.Expect(LogType.Error, new Regex($"{MainQueueDispatcher.ExceptionErrorMessage}.*"));
        }
    }
}