using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NSubstitute;
using NUnit.Framework;
using UnityEngine;

namespace Pitaya.Tests
{
    [TestFixture, Category("unit")]
    public class PitayaClientTest
    {
        public class TestResponse
        {
            public string Attr;
            public List<float> List;

            public override bool Equals(object obj)
            {
                var o = obj as TestResponse;
                if (o == null || obj.GetType() != typeof(TestResponse)) return false;
                else return o.Attr == Attr && o.List.SequenceEqual(List);
            }

            public override int GetHashCode()
            {
                return base.GetHashCode() ^ Attr.GetHashCode() ^ List.GetHashCode();
            }
        }
        
        public class TestRequest {}
        
        [Test]
        public void TestJsonRequestCall()
        {
            var route = "route";

            var expectedRes = new TestResponse
            {
                Attr = "wololo",
                List = new List<float> { 14.3f, 11.0f, 65.4f }
            };
            
            var mockBinding = Substitute.For<IPitayaBinding>();

            var res = Newtonsoft.Json.JsonConvert.SerializeObject(expectedRes);

            var client = new PitayaClient(binding: mockBinding, queueDispatcher: new NullPitayaQueueDispatcher());
            mockBinding.When(x => x.Request(Arg.Any<IntPtr>(), route, Arg.Any<byte[]>(), Arg.Any<uint>(), Arg.Any<int>())).Do(x =>
            {
                client.OnRequestResponse(1, Encoding.UTF8.GetBytes(res));
            });
            
            var completionSource = new TaskCompletionSource<(TestResponse, PitayaError)>();
            client.Request<TestResponse>(route, new TestRequest(), (TestResponse response) =>
            {
                completionSource.TrySetResult((response, null));
            }, (PitayaError error) =>
            {
                completionSource.TrySetResult((null, error));
            });
            
            (TestResponse resp, PitayaError err) = completionSource.Task.GetAwaiter().GetResult();
            Assert.Null(err);
            Assert.AreEqual(expectedRes, resp);
        }
        
        [Test]
        public void TestJsonNotifyCall()
        {
            var route = "route";

            var expectedRes = new TestResponse
            {
                Attr = "wololo",
                List = new List<float> { 14.3f, 11.0f, 65.4f }
            };
            
            var mockBinding = Substitute.For<IPitayaBinding>();

            var res = Newtonsoft.Json.JsonConvert.SerializeObject(expectedRes);

            var client = new PitayaClient(binding: mockBinding, queueDispatcher: new NullPitayaQueueDispatcher());
            
            Assert.DoesNotThrow(() =>
            {
                client.Notify(route, new TestResponse()); 
            });
        }
    }
}