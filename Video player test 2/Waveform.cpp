#include "Waveform.h"
#include "App.h"

void zcom::Waveform::_OnUpdate()
{
    if (!GetVisible())
        return;

    if (ztime::Main() - _lastUpdate < _updateInterval)
        return;
    _lastUpdate = ztime::Main();

    auto adapter = _scene->GetApp()->playback.AudioAdapter();
    if (adapter)
    {
        auto data = adapter->GetRecentSampleData();

        for (int frange = 1; frange < 500; frange++)
        {
            float lAmpComb = 0.0f;
            float rAmpComb = 0.0f;
            //Pos2D<float> lVecComb = 0.0f;
            //Pos2D<float> rVecComb = 0.0f;

            for (int f = frange * 10; f < frange * 10 + 1; f++)
            {
                float lAmp = 0.0f;
                float rAmp = 0.0f;
                //Pos2D<float> lVec = 0.0f;
                //Pos2D<float> rVec = 0.0f;
                for (size_t sample = 0; sample < data.size(); sample++)
                {
                    float t = sample / (float)data[sample].sampleRate;
                    auto point = point_rotated_by({ 0.0f, 0.0f }, { 1.0f, 0.0f }, 2 * 3.1415f * t * f);
                    lAmp += (point * (data[sample].data[0] / 32767.0f)).x;
                    rAmp += (point * (data[sample].data[1] / 32767.0f)).x;
                    //lVec += point * (data[sample].data[0] / 32767.0f);
                    //rVec += point * (data[sample].data[1] / 32767.0f);
                    //lPoints[sample - tail] = point * (lData[sample] / 32767.0f);
                    //rPoints[sample - tail] = point * (rData[sample] / 32767.0f);
                }

                lAmp /= data.size();
                rAmp /= data.size();
                //lVec = lVec / float(data.size());
                //rVec = rVec / float(data.size());

                lAmpComb += lAmp;
                rAmpComb += rAmp;
                //lVecComb += lVec;
                //rVecComb += rVec;
            }

            lAmpComb /= 1.0f;
            rAmpComb /= 1.0f;
            //lVecComb = lVecComb / 1.0f;
            //rVecComb = rVecComb / 1.0f;

            _data.lAmps[frange - 1] = lAmpComb;
            _data.rAmps[frange - 1] = rAmpComb;
        }
    }
    else
    {
        for (int i = 0; i < 500; i++)
        {
            _data.lAmps[i] = 0;
            _data.rAmps[i] = 0;
        }
    }
}