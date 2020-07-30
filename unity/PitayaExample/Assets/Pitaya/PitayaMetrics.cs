using System;

namespace Pitaya
{
    public class PitayaMetrics
    {
        private int[] _rttSamples;
        private int _validPings;
        public PitayaMetrics()
        {
            _rttSamples = new int[3];
            _validPings = 0;
        }

        public void PingReceived(int ping)
        {
            if (ping < 0) return;
            if (_validPings < 3) _validPings++;
            _rttSamples[2] = _rttSamples[1];
            _rttSamples[1] = _rttSamples[0];
            _rttSamples[0] = ping;

        }

        public int GetRTT()
        {
            if (_validPings == 0) return -1;
            if (_validPings == 1) return _rttSamples[0];
            else if (_validPings == 2) {
                return (_rttSamples[1] + _rttSamples[0]) / 2;
            }
            int highest = Math.Max(_rttSamples[2], _rttSamples[1]);
            highest = Math.Max(highest, _rttSamples[0]);
            return (_rttSamples[2] + _rttSamples[1] + _rttSamples[0] - highest) / 2;
        }

    }



}