using Newtonsoft.Json;
using NUnit.Framework;

namespace Pitaya.Tests
{
    [TestFixture, Category("unit")]
    public class SerializerFactoryTest
    {
        public struct TestStruct
        {
            public string ValueA;
            public float[] ValueB;
        }
        
        [Test]
        public void TestCreateJsonSerializer()
        {
            var strct = new TestStruct
            {
                ValueA = "foo",
                ValueB = new float[] {23.4f, 12.3f},
            };
            
            var settings = new JsonSerializerSettings();
            settings.Formatting = Formatting.Indented;
            var expectedSerializer = new JsonSerializer(settings); 
            
            var factory = new SerializerFactory(settings);
            var serializer = factory.CreateJsonSerializer();
            
            Assert.AreEqual(expectedSerializer.Encode(strct), serializer.Encode(strct));
        }
    }
}