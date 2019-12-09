using System;
using System.Diagnostics;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Assertions;

namespace Pitaya
{
    public class PitayaMetrics
    {
        public delegate void MetricsCallback(ConnectionSessionStats stats);

        public enum PingStatus
        {
            Ok,
            Timeout,
            Error
        }

        private class RequestRecording
        {
            public List<double> LatenciesMs = new List<double>();
            public Stopwatch Watch = new Stopwatch();
        }

        public delegate void SendPing(Action<PingStatus> onPingDone);

        private enum State
        {
            NotConnected,
            Connecting,
            Connected
        }

        private class ConnectingState
        {
            public Stopwatch ConnectionWatch = new Stopwatch();
        }

        private class ConnectedState
        {
            public Stopwatch SessionWatch = new Stopwatch();
            public bool KickReceived = false;
        }

        // The current version of the event that is being sent. This value should always increase when the
        // format of the struct changes.
        private const uint EventVersion = 1;

        // The current state of the pitaya connection.
        private State _state;
        private ConnectedState _connectedState;
        private ConnectingState _connectingState;
        private Dictionary<string, RequestRecording> _requestsLatencies = new Dictionary<string, RequestRecording>(15);

        private ConnectionSessionStats _connectionSessionStats;
        private readonly MetricsCallback _cb;

        private static class ConnectionFinishReason
        {
            public const string UserRequest = "UserRequest";
            public const string FailedToConnect = "FailedToConnect";
            public const string ConnectionError = "ConnectionError";
            public const string Kick = "Kick";
            public const string UnknownError = "UnknownError";
        }

        public struct ConnectionSessionStats
        {
            // TODO(lhahn): Consider the case where multiple clients are created, should a session contain an ID?
            // Or should a pitaya client contain an id as well to distinguish different client instances?
            public uint Version;
            public double SessionDurationSec;
            public uint PingAverage;
            public uint PingStdDeviation;
            public uint PingTotal;
            public uint PingLoss;
            public string ConnectionFinishReason;
            public string ConnectionFinishDetails;
            public double ConnectionTimeMs;
            public string ConnectionRegion;
            public Dictionary<string, double> RoutesLatencyMs;
            public Dictionary<string, double> RoutesStandardDeviation;
            public string NetworkType;
            public string LibPitayaVersion;
            public uint ServerInvalidPackages;

            public string Serialize()
            {
                return SimpleJson.SimpleJson.SerializeObject(this);
            }
        }

        public PitayaMetrics(MetricsCallback metricsCB)
        {
            Assert.IsNotNull(metricsCB);
            _cb = metricsCB;
            _state = State.NotConnected;
            _connectedState = null;
            _connectingState = null;
        }

        public void Start()
        {
            _connectionSessionStats = DefaultConnectionSessionStats();
            _state = State.Connecting;
            _connectingState = new ConnectingState
            {
                ConnectionWatch = new Stopwatch()
            };
            _connectingState.ConnectionWatch.Start();
            Assert.IsNull(_connectedState);
        }

        public void StartRecordingRequest(string route)
        {
            // We should not assume here that the _state variable will be of a specific value. LibPitaya can buffer
            // requests even before the client is connected.
            if (_requestsLatencies.TryGetValue(route, out RequestRecording recording))
            {
                recording.Watch.Start();
            }
            else
            {
                var r = new RequestRecording();
                _requestsLatencies.Add(route, r);
                r.Watch.Start();
            }
        }

        public void StopRecordingRequest(string route, PitayaError err = null)
        {
            if (err != null)
            {
                // TODO(lhahn): Should some errors not be collected here? For example, timeouts.
                // for the moment just ignore errors...
            }
            
            Assert.IsTrue(_requestsLatencies.ContainsKey(route));
            if (_requestsLatencies.TryGetValue(route, out RequestRecording recording))
            {
                recording.Watch.Stop();
                recording.LatenciesMs.Add(recording.Watch.Elapsed.TotalMilliseconds);
                recording.Watch.Reset();
            }
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
            Assert.IsNotNull(_connectedState);
            Assert.IsNull(_connectingState);

            switch (pitayaState)
            {
                case PitayaNetWorkState.Kicked:
                    // LibPitaya sends a Kicked event and after that a Disconnected event. Therefore,
                    // we do not close the session yet, we just signal that a kick was received.
                    _connectedState.KickReceived = true;
                    break;
                case PitayaNetWorkState.Disconnected:
                    StopConnectedState(pitayaErr);
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
            Assert.IsNull(_connectedState);
            Assert.IsNull(_connectingState);

            if (pitayaState != PitayaNetWorkState.Disconnected)
            {
                // It is possible to receive a duplicated disconnected event, so we just ignore it.
                throw new Exception(string.Format("PitayaMetrics received pitaya state {0} when in not connected state", pitayaState));
            }
        }

        private void UpdateConnectingState(PitayaNetWorkState pitayaState, NetworkError pitayaErr)
        {
            Assert.IsTrue(_state == State.Connecting);
            Assert.IsNull(_connectedState);
            Assert.IsNotNull(_connectingState);

            switch (pitayaState)
            {
                case PitayaNetWorkState.Connected:
                    // If the connection connected successfully, we start the session stopwatch and
                    // stop the connection watch.
                    _connectingState.ConnectionWatch.Stop();
                    _connectionSessionStats.ConnectionTimeMs = _connectingState.ConnectionWatch.Elapsed.TotalMilliseconds;
                    _connectingState.ConnectionWatch.Reset();
                    _connectingState = null;

                    _state = State.Connected;
                    _connectedState = new ConnectedState();
                    _connectedState.SessionWatch.Start();
                    break;
                case PitayaNetWorkState.FailToConnect:
                    // If the connection failed while we were trying to connect, we should close the session with
                    // this information.
                    StopConnectingState(pitayaErr);
                    break;
                case PitayaNetWorkState.Error:
                    // This event only happens when unknown data from the server was sent, so we just increment the counter.
                    _connectionSessionStats.ServerInvalidPackages++;
                    break;
                default:
                    throw new Exception(string.Format("PitayaMetrics received pitaya state {0} when in not connected state", pitayaState));
            }
        }

        public void ForceStop()
        {
            // HACK(lhahn): We simulate a disconnect to the metrics aggregator. This is necessary because the dispose is called
            // before the disconnect event can be fired in the PitayaClient class. This could be resolved in the future,
            // but for the moment I think this solution won't have issues.
            Update(PitayaNetWorkState.Disconnected, null);
        }

        private void StopConnectingState(NetworkError pitayaErr)
        {
            Assert.IsNotNull(pitayaErr);
            Assert.IsNull(_connectedState);
            Assert.IsNotNull(_connectingState);
            _connectionSessionStats.ConnectionFinishReason = ConnectionFinishReason.FailedToConnect;
            _connectionSessionStats.ConnectionFinishDetails = GetErrorDetails(pitayaErr);
            CalculateRoutesMetrics(ref _connectionSessionStats);

            SendConnectionStatsSummaryAndResetState();
        }

        private void StopConnectedState(NetworkError pitayaErr)
        {
            Assert.IsNotNull(_connectedState);
            Assert.IsNull(_connectingState);

            if (pitayaErr == null)
            {
                _connectionSessionStats.ConnectionFinishReason = _connectedState.KickReceived
                    ? ConnectionFinishReason.Kick
                    : ConnectionFinishReason.UserRequest;
            }
            else
            {
                _connectionSessionStats.ConnectionFinishReason = ConnectionFinishReason.ConnectionError;
                _connectionSessionStats.ConnectionFinishDetails = GetErrorDetails(pitayaErr);
            }

            _connectedState.SessionWatch.Stop();
            _connectionSessionStats.SessionDurationSec = _connectedState.SessionWatch.Elapsed.TotalSeconds;
            CalculateRoutesMetrics(ref _connectionSessionStats);

            SendConnectionStatsSummaryAndResetState();
        }

        private void SendConnectionStatsSummaryAndResetState()
        {
            _cb(_connectionSessionStats);
            _connectionSessionStats = DefaultConnectionSessionStats();
            _state = State.NotConnected;
            _connectingState = null;
            _connectedState = null;
            // TODO(lhahn): consider not clearing the dictionary here, since the routes will probably be reused anyways.
            _requestsLatencies.Clear();
        }

        private void CalculateRoutesMetrics(ref ConnectionSessionStats connectionSessionStats)
        {
            Assert.IsTrue(connectionSessionStats.RoutesLatencyMs.Count == 0);
            Assert.IsTrue(connectionSessionStats.RoutesStandardDeviation.Count == 0);

            foreach (var kv in _requestsLatencies)
            {
                string route = kv.Key;
                RequestRecording recording = kv.Value;

                // Calculate the average
                double averageLatency = 0;
                for (var i = 0; i < recording.LatenciesMs.Count; ++i)
                {
                    averageLatency += recording.LatenciesMs[i];
                }
                averageLatency /= recording.LatenciesMs.Count;
                
                // Calculate the standard deviation
                double stdDeviation = 0;
                for (var i = 0; i < recording.LatenciesMs.Count; ++i)
                {
                    stdDeviation += Math.Pow(recording.LatenciesMs[i] - averageLatency, 2);
                }
                stdDeviation /= recording.LatenciesMs.Count;
                stdDeviation = Math.Sqrt(stdDeviation);

                connectionSessionStats.RoutesLatencyMs.Add(route, averageLatency);
                connectionSessionStats.RoutesStandardDeviation.Add(route, stdDeviation);
            }
        }

        private ConnectionSessionStats DefaultConnectionSessionStats()
        {
            return new ConnectionSessionStats
            {
                Version = EventVersion,
                NetworkType = GetNetworkType(),
                LibPitayaVersion = PitayaBinding.Version,
                // TODO(lhahn): remove hardcoded region here and use something better.
                ConnectionRegion = "NA",
                RoutesLatencyMs = new Dictionary<string, double>(),
                RoutesStandardDeviation = new Dictionary<string, double>()
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
