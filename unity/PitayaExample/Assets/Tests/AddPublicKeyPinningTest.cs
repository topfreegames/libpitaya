using System;
using System.Collections;
using System.IO;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Pitaya.Tests
{
    public class AddPublicKeyPinningTest
    {
        private const string ServerHost = "libpitaya-tests.tfgco.com";
        private const int ServerPort = 3252;
        private string _serverCertPath;
        private string _caPath;
        private string _corruptCaPath;

        private PitayaBinding _binding;

        [SetUp]
        public void Init()
        {
            _binding = new PitayaBinding();
            _corruptCaPath = "corrupt-ca.crt";
            _caPath = "myCA.pem";
            _serverCertPath = "client-ssl.localhost.crt";
        }

        [TearDown]
        public void TearDown()
        {
            _binding.ClearPinnedPublicKeys();
        }

        [UnityTest]
        public IEnumerator ShouldThrowInCaseOfErrorsWhenAddingKey()
        {
            Assert.Catch<Exception>(() =>
                _binding.AddPinnedPublicKeyFromCertificateFile("non-existent-path"));
            Assert.Catch<Exception>(() =>
                _binding.AddPinnedPublicKeyFromCertificateFile(_corruptCaPath));

            var certText = File.ReadAllText(Path.Combine(Application.streamingAssetsPath, _corruptCaPath));
            Assert.Catch<Exception>(() => _binding.AddPinnedPublicKeyFromCertificateString(certText));

            return null;
        }

        [UnityTest]
        public IEnumerator ShouldWorkWithCorrectCertificates()
        {
            Assert.DoesNotThrow(() => _binding.AddPinnedPublicKeyFromCertificateFile(_caPath));
            Assert.DoesNotThrow(() => _binding.AddPinnedPublicKeyFromCertificateFile(_serverCertPath));

            var caText = File.ReadAllText(Path.Combine(Application.streamingAssetsPath, _caPath));
            var certText = File.ReadAllText(Path.Combine(Application.streamingAssetsPath, _serverCertPath));

            Assert.DoesNotThrow(() => _binding.AddPinnedPublicKeyFromCertificateString(caText));
            Assert.DoesNotThrow(() => _binding.AddPinnedPublicKeyFromCertificateString(certText));

            return null;
        }
    }
}
