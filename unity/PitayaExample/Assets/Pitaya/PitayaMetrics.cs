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
        public ConnectionSessionStats connectionSessionStats;
        public delegate void MetricsCallback(ConnectionSessionStats stats);
        private Stopwatch connectionWatch = new Stopwatch();
        private Stopwatch sessionWatch = new Stopwatch();
        private MetricsCallback cb;

        public enum ConnectionFailure
        {
            CouldNotResolveHost,
            Error,
            Timeout,
        }

        private static class disconnectionReason
        {
            public const string ConnectionEndedNormally = "ConnectionEndedNormally";
            public const string ConnectionTimeout = "ConnectionTimeout";
            public const string FailedToConnect = "FailedToConnect";
            public const string ConnectionErrored = "ConnectionErrored";
            public const string ConnectionClosed = "ConnectionClosed";
            public const string ConnectionKicked = "Kicked";
        }

        private PitayaNetWorkState[] sessionStartedStates = {
            PitayaNetWorkState.Connected, PitayaNetWorkState.FailToConnect, PitayaNetWorkState.Timeout, PitayaNetWorkState.Error
        };

        private PitayaNetWorkState[] sessionErroredStates = {
            PitayaNetWorkState.FailToConnect, PitayaNetWorkState.Timeout, PitayaNetWorkState.Error
        };

        private PitayaNetWorkState[] sessionEndStates = {
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
            cb = metricsCB;
        }

        public void Start()
        {
            connectionSessionStats = new ConnectionSessionStats();
            sessionWatch.Start();
            connectionWatch.Start();
        }

        public void Update(PitayaNetWorkState state, NetworkError error)
        {
            // Session started
            if (sessionStartedStates.Contains(state))
            {
                connectionWatch.Stop();
                connectionSessionStats.ConnectionTime = connectionWatch.Elapsed;
            }

            // Session ended
            if (sessionEndStates.Contains(state) || sessionErroredStates.Contains(state))
            {
                stop(state, error);
            }
        }

        private void stop(PitayaNetWorkState state=PitayaNetWorkState.Disconnected, NetworkError error=null)
        {
            sessionWatch.Stop();
            connectionSessionStats.SessionDurationSec = sessionWatch.Elapsed;
            setDisconnectionReason(state, error);

            if (cb != null)
            {
                cb(connectionSessionStats);
            }
        }

        private void setDisconnectionReason(PitayaNetWorkState state, NetworkError error)
        {
            if (state == PitayaNetWorkState.Disconnected)
            {
                // Disconnected with errors
                if (error != null) {
                    connectionSessionStats.DisconnectionReason = error.Error;
                    return;
                }

                // Disconnected normally
                connectionSessionStats.DisconnectionReason = disconnectionReason.ConnectionEndedNormally;
                return;
            }

            // Timeout trying to connect
            if (state == PitayaNetWorkState.Timeout)
            {
                connectionSessionStats.DisconnectionReason = disconnectionReason.ConnectionTimeout;
                connectionSessionStats.ConnectionFailure = ConnectionFailure.Timeout;
                if (error != null) {
                    connectionSessionStats.ConnectionFailureDetails = error.Error;
                }
                return;
            }

            // Disconnected due to errors
            if (state == PitayaNetWorkState.Error || state == PitayaNetWorkState.FailToConnect)
            {
                connectionSessionStats.ConnectionFailure = ConnectionFailure.Error;
                connectionSessionStats.DisconnectionReason = disconnectionReason.ConnectionErrored;

                if (state == PitayaNetWorkState.FailToConnect)
                {
                    connectionSessionStats.DisconnectionReason = disconnectionReason.FailedToConnect;
                }

                // Error string is set
                if (error != null) {
                    connectionSessionStats.ConnectionFailureDetails = error.Error;
                    return;
                }
                
                return;
            }

            // Closed connection
            if (state == PitayaNetWorkState.Closed)
            {
                connectionSessionStats.DisconnectionReason = disconnectionReason.ConnectionClosed;
                return;
            }

            // Kicked
            if (state == PitayaNetWorkState.Kicked)
            {
                connectionSessionStats.DisconnectionReason = disconnectionReason.ConnectionKicked;
                return;
            }
        }
    }
}
