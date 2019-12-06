using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using Google.Protobuf;
using UnityEngine;
using System.Linq;
using UnityEngine.Assertions;

namespace Pitaya
{
    public class PitayaMetrics
    {
        public delegate void MetricsCallback(ConnectionSessionStats stats);

        private enum State
        {
            NotConnected,
            Connecting,
            Connected
        }

        // The current version of the event that is being sent. This value should always increase when the
        // format of the struct changes.
        private const uint EventVersion = 1;
        
        // The current state of the pitaya connection.
        private State _state;

        private ConnectionSessionStats _connectionSessionStats;
        private readonly Stopwatch _connectionWatch;
        private readonly Stopwatch _sessionWatch;
        private readonly MetricsCallback _cb;
        private bool _kickReceived;

        private static class ConnectionFinishReason
        {
            public const string UserRequest = "UserRequest";
            public const string FailedToConnect = "FailedToConnect";
            public const string ConnectionError = "ConnectionError";
            public const string Kick = "Kick";
            public const string UnknownError = "UnknownError";
        }

        public struct PingStats
        {
            public uint Average;
            public uint StandardDeviation;
            public uint Loss;
        }

        public struct ConnectionSessionStats
        {
            // TODO(lhahn): Consider the case where multiple clients are created, should a session contain an ID?
            // Or should a pitaya client contain an id as well to distinguish different client instances?
            public uint Version;
            public double? SessionDurationSec;
            public PingStats? Ping;
            public string ConnectionFinishReason;
            public string ConnectionFinishDetails;
            public double ConnectionTimeMs;
            public string ConnectionRegion;
            public Dictionary<string, TimeSpan> RoutesLatency;
            public Dictionary<string, TimeSpan> RoutesStandardDeviation;
            public string NetworkType;
            public string LibPitayaVersion;
            public uint ServerInvalidPackages;

            public string Serialize()
            {
                return SimpleJson.SimpleJson.SerializeObject(this);
            }
        }

        public PitayaMetrics(MetricsCallback metricsCB = null)
        {
            _cb = metricsCB;
            _state = State.NotConnected;
            _connectionWatch = new Stopwatch();
            _sessionWatch = new Stopwatch();
            _kickReceived = false;
        }

        public void Start()
        {
            _connectionSessionStats = DefaultConnectionSessionStats();
            _connectionWatch.Start();
            _state = State.Connecting;
            _kickReceived = false;
        }

        public void Update(PitayaNetWorkState pitayaState, NetworkError error)
        {
            switch (_state)
            {
                case State.NotConnected:
                    UpdateNotConnectedState(pitayaState, error);
                    break;
                case State.Connecting:
                    UpdateConnectingState(pitayaState, error);
                    break;
                case State.Connected:
                    UpdateConnectedState(pitayaState, error);
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private void UpdateConnectedState(PitayaNetWorkState pitayaState, NetworkError pitayaErr)
        {
            Assert.IsTrue(_state == State.Connected);

            switch (pitayaState)
            {
                case PitayaNetWorkState.Kicked:
                    // LibPitaya sends a Kicked event and after that a Disconnected event. Therefore,
                    // we do not close the session yet, we just signal that a kick was received.
                    _kickReceived = true;
                    break;
                case PitayaNetWorkState.Disconnected:
                    if (pitayaErr == null)
                    {
                        _connectionSessionStats.ConnectionFinishReason = _kickReceived
                            ? ConnectionFinishReason.Kick
                            : ConnectionFinishReason.UserRequest;
                    }
                    else
                    {
                        _connectionSessionStats.ConnectionFinishReason = ConnectionFinishReason.ConnectionError;
                        _connectionSessionStats.ConnectionFinishDetails = GetErrorDetails(pitayaErr);
                    }
                    StopSession();
                    break;
                case PitayaNetWorkState.Error:
                    // This event only happens when unknown data from the server was sent, so we just increment the counter.
                    _connectionSessionStats.ServerInvalidPackages++;
                    break;
                default:
                    throw new Exception(string.Format("PitayaMetrics received pitaya state {0} when in not connected state", pitayaState));
            }
        }

        private void UpdateNotConnectedState(PitayaNetWorkState pitayaState, NetworkError pitayaErr)
        {
            Assert.IsTrue(_state == State.NotConnected);
            throw new Exception(string.Format("PitayaMetrics received pitaya state {0} when in not connected state", pitayaState));
        }

        private void UpdateConnectingState(PitayaNetWorkState pitayaState, NetworkError pitayaErr)
        {
            Assert.IsTrue(_state == State.Connecting);

            switch (pitayaState)
            {
                case PitayaNetWorkState.Connected:
                    // If the connection connected successfully, we start the session stopwatch and
                    // stop the connection watch.
                    _connectionWatch.Stop();
                    _connectionSessionStats.ConnectionTimeMs = _connectionWatch.Elapsed.TotalMilliseconds;
                    _connectionWatch.Reset();
                    _sessionWatch.Start();

                    _state = State.Connected;
                    break;
                case PitayaNetWorkState.FailToConnect:
                    // If the connection failed while we were trying to connect, we should close the session with
                    // this information.
                    _connectionSessionStats.ConnectionFinishReason = ConnectionFinishReason.FailedToConnect;
                    _connectionSessionStats.ConnectionFinishDetails = GetErrorDetails(pitayaErr);
                    StopSession();
                    break;
                case PitayaNetWorkState.Error:
                    // This event only happens when unknown data from the server was sent, so we just increment the counter.
                    _connectionSessionStats.ServerInvalidPackages++;
                    break;
                default:
                    throw new Exception(string.Format("PitayaMetrics received pitaya state {0} when in not connected state", pitayaState));
            }
        }

        private void StopSession()
        {
            _sessionWatch.Stop();
            _connectionSessionStats.SessionDurationSec = _sessionWatch.Elapsed.TotalSeconds;
            _sessionWatch.Reset();

            _cb(_connectionSessionStats);
            _connectionSessionStats = DefaultConnectionSessionStats();
            _kickReceived = false;
            _state = State.NotConnected;
        }

        private ConnectionSessionStats DefaultConnectionSessionStats()
        {
            return new ConnectionSessionStats
            {
                Version = EventVersion,
                ServerInvalidPackages = 0,
                NetworkType = GetNetworkType(),
                LibPitayaVersion = PitayaBinding.Version,
                // TODO(lhahn): remove hardcoded region here and use something better.
                ConnectionRegion = "NA"
            };
        }

        private static string GetNetworkType()
        {
            switch (Application.internetReachability)
            {
                case NetworkReachability.NotReachable: return "not-reachable";
                case NetworkReachability.ReachableViaCarrierDataNetwork: return "data";
                case NetworkReachability.ReachableViaLocalAreaNetwork: return "wifi";
                default: throw new ArgumentOutOfRangeException();
            }
        }

        private static string GetErrorDetails(NetworkError e)
        {
            Assert.IsNotNull(e, "error should not be null");
            return string.Format("{0}: {1}", e.Error, e.Description);
        }
    }
}
