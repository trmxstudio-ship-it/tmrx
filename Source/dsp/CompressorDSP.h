#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace tmrx
{

/**
    JUCE 에 의존하지 않는 피드포워드 컴프레서 DSP.

    - 스테레오 링크 디텍터 (채널 중 가장 큰 레벨 기준)
    - Peak / RMS 디텍터 선택
    - 소프트 니 (Giannoulis, Massberg & Reiss, 2012)
    - 로그 도메인에서 attack / release 스무딩
    - 메이크업 게인, Dry/Wet 믹스 (둘 다 파라미터 스무딩)
*/
class CompressorDSP
{
public:
    struct Parameters
    {
        float thresholdDb = -18.0f;
        float ratio       = 4.0f;
        float kneeDb      = 6.0f;
        float attackMs    = 10.0f;
        float releaseMs   = 120.0f;
        float makeupDb    = 0.0f;
        float mix         = 1.0f;   // 0 = dry, 1 = wet
        bool  rmsDetector = false;
    };

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        rmsCoeff    = timeToCoeff (10.0f);  // 10 ms RMS 윈도우
        smoothCoeff = timeToCoeff (20.0f);  // 메이크업/믹스 스무딩
        updateCoefficients();
        reset();
    }

    void reset()
    {
        envelopeDb   = 0.0f;
        rmsState     = 0.0f;
        makeupSmooth = params.makeupDb;
        mixSmooth    = params.mix;
    }

    void setParameters (const Parameters& p)
    {
        params = p;
        params.ratio   = std::max (1.0f, params.ratio);
        params.kneeDb  = std::max (0.0f, params.kneeDb);
        params.mix     = std::clamp (params.mix, 0.0f, 1.0f);
        updateCoefficients();
    }

    const Parameters& getParameters() const noexcept { return params; }

    /** 정적 게인 커브: 입력 레벨(dB) -> 출력 레벨(dB). */
    static float computeOutputLevelDb (float inDb, float thresholdDb, float ratio, float kneeDb) noexcept
    {
        const float over = inDb - thresholdDb;

        if (2.0f * over < -kneeDb)
            return inDb;

        if (kneeDb > 0.0f && 2.0f * std::abs (over) <= kneeDb)
        {
            const float t = over + kneeDb * 0.5f;
            return inDb + (1.0f / ratio - 1.0f) * t * t / (2.0f * kneeDb);
        }

        return thresholdDb + over / ratio;
    }

    /**
        오디오를 in-place 로 처리합니다.
        반환값: 이 블록에서 발생한 최대 게인 리덕션(dB, 양수).
    */
    float process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        float maxReductionDb = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // 1) 디텍터: 스테레오 링크
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                peak = std::max (peak, std::abs (channels[ch][i]));

            float level = peak;
            if (params.rmsDetector)
            {
                rmsState = rmsCoeff * rmsState + (1.0f - rmsCoeff) * peak * peak;
                level = std::sqrt (rmsState);
            }

            // 2) 게인 컴퓨터
            const float inDb     = gainToDb (level);
            const float targetDb = computeOutputLevelDb (inDb, params.thresholdDb, params.ratio, params.kneeDb) - inDb;

            // 3) attack / release (로그 도메인, 게인 리덕션은 음수)
            const float coeff = targetDb < envelopeDb ? attackCoeff : releaseCoeff;
            envelopeDb = coeff * envelopeDb + (1.0f - coeff) * targetDb;

            // 4) 메이크업 & 믹스
            makeupSmooth = smoothCoeff * makeupSmooth + (1.0f - smoothCoeff) * params.makeupDb;
            mixSmooth    = smoothCoeff * mixSmooth    + (1.0f - smoothCoeff) * params.mix;

            const float wetGain = dbToGain (envelopeDb + makeupSmooth);
            const float gain    = (1.0f - mixSmooth) + mixSmooth * wetGain;

            for (int ch = 0; ch < numChannels; ++ch)
                channels[ch][i] *= gain;

            maxReductionDb = std::max (maxReductionDb, -envelopeDb);
        }

        // 디노멀 방지
        if (std::abs (envelopeDb) < 1.0e-6f) envelopeDb = 0.0f;
        if (rmsState < 1.0e-12f)             rmsState   = 0.0f;

        return maxReductionDb;
    }

    static float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-6f)); }
    static float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

private:
    float timeToCoeff (float ms) const noexcept
    {
        if (ms <= 0.0f)
            return 0.0f;
        return static_cast<float> (std::exp (-1.0 / (0.001 * ms * sampleRate)));
    }

    void updateCoefficients() noexcept
    {
        attackCoeff  = timeToCoeff (params.attackMs);
        releaseCoeff = timeToCoeff (params.releaseMs);
    }

    Parameters params;
    double sampleRate = 44100.0;

    float attackCoeff = 0.0f, releaseCoeff = 0.0f, rmsCoeff = 0.0f, smoothCoeff = 0.0f;
    float envelopeDb = 0.0f, rmsState = 0.0f, makeupSmooth = 0.0f, mixSmooth = 1.0f;
};

} // namespace tmrx
