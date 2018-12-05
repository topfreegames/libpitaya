using System;
using System.Collections;
using System.IO;
using System.Threading;
using NUnit.Framework;
using SimpleJson;
using UnityEngine;
using UnityEngine.TestTools;

namespace Pitaya.Tests
{
    public class AddPublicKeyPinningTest
    {
        private const string ServerHost = "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com";
        private const int ServerPort = 3252;
        private string _serverCertPath;
        private string _caPath;
        private string _corruptCaPath;

        [SetUp]
        public void Init()
        {
            _corruptCaPath = Path.Combine(Application.streamingAssetsPath, "corrupt-ca.crt");
            _caPath = Path.Combine(Application.streamingAssetsPath, "ca.crt");
            _serverCertPath = Path.Combine(Application.streamingAssetsPath, "client-ssl.localhost.crt");
        }

        [TearDown]
        public void TearDown()
        {
            PitayaBinding.ClearPinnedPublicKeys();
        }

        [UnityTest]
        public IEnumerator ShouldThrowInCaseOfErrorsWhenAddingKey()
        {
            Assert.Catch<Exception>(() =>
                PitayaBinding.AddPinnedPublicKeyFromCertificateFile("non-existent-path"));
            Assert.Catch<Exception>(() =>
                PitayaBinding.AddPinnedPublicKeyFromCertificateFile(_corruptCaPath));

            var certText = File.ReadAllText(_corruptCaPath);
            Assert.Catch<Exception>(() => PitayaBinding.AddPinnedPublicKeyFromCertificateString(certText));

            return null;
        }

        [UnityTest]
        public IEnumerator ShouldWorkWithCorrectCertificates()
        {
            Assert.DoesNotThrow(() => PitayaBinding.AddPinnedPublicKeyFromCertificateFile(_caPath));
            Assert.DoesNotThrow(() => PitayaBinding.AddPinnedPublicKeyFromCertificateFile(_serverCertPath));

            var caText = File.ReadAllText(_caPath);
            var certText = File.ReadAllText(_serverCertPath);

            Assert.DoesNotThrow(() => PitayaBinding.AddPinnedPublicKeyFromCertificateString(caText));
            Assert.DoesNotThrow(() => PitayaBinding.AddPinnedPublicKeyFromCertificateString(certText));

            return null;
        }
    }
}
