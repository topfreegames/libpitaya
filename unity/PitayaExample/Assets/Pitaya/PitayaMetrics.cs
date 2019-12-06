using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using Google.Protobuf;
using UnityEngine;
using System.Linq;

namespace Pitaya
{
    public class PitayaMetrics
    {
        public delegate void MetricsCallback(ConnectionSessionStats stats);

        public ConnectionSessionStats connectionSessionStats;
        private Stopwatch _connectionWatch = new Stopwatch();
        private Stopwatch _sessionWatch = new Stopwatch();
        private MetricsCallback _cb;

        public enum ConnectionFailure
        {
            CouldNotResolveHost,
            Error,
            Timeout,
        }

        private static class DisconnectionReason
        {
            public const string ConnectionEndedNormally = "ConnectionEndedNormally";
            public const string ConnectionTimeout = "ConnectionTimeout";
            public const string FailedToConnect = "FailedToConnect";
            public const string ConnectionErrored = "ConnectionErrored";
            public const string ConnectionClosed = "ConnectionClosed";
            public const string ConnectionKicked = "Kicked";
        }

        private static readonly PitayaNetWorkState[] SessionStartedStates = {
            PitayaNetWorkState.Connected, PitayaNetWorkState.FailToConnect, PitayaNetWorkState.Timeout, PitayaNetWorkState.Error
        };

        private static readonly PitayaNetWorkState[] SessionErroredStates = {
            PitayaNetWorkState.FailToConnect, PitayaNetWorkState.Timeout, PitayaNetWorkState.Error
        };

        private static readonly PitayaNetWorkState[] SessionEndStates = {
            PitayaNetWorkState.Disconnected, PitayaNetWorkState.Kicked, PitayaNetWorkState.Closed
        };

        public struct PingStats
        {
            public uint Average;
            public uint StandardDeviation;
            public uint Loss;
        }

        public struct ConnectionSessionStats
        {
            public uint Version;
            public TimeSpan? SessionDurationSec;
            public PingStats? Ping;
            public string DisconnectionReason;
            public ConnectionFailure ConnectionFailure;
            public string ConnectionFailureDetails;
            public TimeSpan ConnectionTime;
            public string ConnectionRegion;
            public Dictionary<string, TimeSpan> RoutesLatency;
            public Dictionary<string, TimeSpan> RoutesStandardDeviation;
            public string NetworkType;
            public string LibPitayaVersion;
        }

        public PitayaMetrics(MetricsCallback metricsCB = null)
        {
            _cb = metricsCB;
        }

        public void Start()
        {
            connectionSessionStats = new ConnectionSessionStats();
            _sessionWatch.Start();
            _connectionWatch.Start();
        }

        public void Update(PitayaNetWorkState state, NetworkError error)
        {
            // Session started
            if (SessionStartedStates.Contains(state))
            {
                _connectionWatch.Stop();
                connectionSessionStats.ConnectionTime = _connectionWatch.Elapsed;
            }

            // Session ended
            if (SessionEndStates.Contains(state) || SessionErroredStates.Contains(state))
            {
                Stop(state, error);
            }
        }

        private void Stop(PitayaNetWorkState state=PitayaNetWorkState.Disconnected, NetworkError error=null)
        {
            _sessionWatch.Stop();
            connectionSessionStats.SessionDurationSec = _sessionWatch.Elapsed;
            SetDisconnectionReason(state, error);

            _cb?.Invoke(connectionSessionStats);
        }

        private void SetDisconnectionReason(PitayaNetWorkState state, NetworkError error)
        {
            if (state == PitayaNetWorkState.Disconnected)
            {
                // Disconnected with errors
                if (error != null)
                {
                    connectionSessionStats.DisconnectionReason = error.Error;
                    return;
                }

                // Disconnected normally
                connectionSessionStats.DisconnectionReason = DisconnectionReason.ConnectionEndedNormally;
                return;
            }

            // Timeout trying to connect
            if (state == PitayaNetWorkState.Timeout)
            {
                connectionSessionStats.DisconnectionReason = DisconnectionReason.ConnectionTimeout;
                connectionSessionStats.ConnectionFailure = ConnectionFailure.Timeout;
                if (error != null)
                {
                    connectionSessionStats.ConnectionFailureDetails = error.Error;
                }
                return;
            }

            // Disconnected due to errors
            if (state == PitayaNetWorkState.Error || state == PitayaNetWorkState.FailToConnect)
            {
                connectionSessionStats.ConnectionFailure = ConnectionFailure.Error;
                connectionSessionStats.DisconnectionReason = DisconnectionReason.ConnectionErrored;

                if (state == PitayaNetWorkState.FailToConnect)
                {
                    connectionSessionStats.DisconnectionReason = DisconnectionReason.FailedToConnect;
                }

                // Error string is set
                if (error != null)
                {
                    connectionSessionStats.ConnectionFailureDetails = error.Error;
                    return;
                }

                return;
            }

            // Closed connection
            if (state == PitayaNetWorkState.Closed)
            {
                connectionSessionStats.DisconnectionReason = DisconnectionReason.ConnectionClosed;
                return;
            }

            // Kicked
            if (state == PitayaNetWorkState.Kicked)
            {
                connectionSessionStats.DisconnectionReason = DisconnectionReason.ConnectionKicked;
            }
        }
    }
}
