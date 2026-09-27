// JUCE 없이 빌드되는 간단한 DSP 테스트
#include "dsp/CompressorDSP.h"

#include <cmath>
#include <cstdio>
#include <vector>

using tmrx::CompressorDSP;

static int failures = 0;

static void expectNear (const char* what, float actual, float expected, float tol)
{
    if (std::abs (actual - expected) > tol)
    {
        std::printf ("FAIL: %s  expected %.4f, got %.4f\n", what, expected, actual);
        ++failures;
    }
    else
    {
        std::printf ("ok:   %s  (%.4f)\n", what, actual);
    }
}

// 일정한 레벨의 DC 신호를 오래 흘려서 정상상태 출력 레벨(dB)을 측정
static float steadyStateOutputDb (CompressorDSP::Parameters p, float inputDb, double fs = 48000.0)
{
    CompressorDSP comp;
    comp.setParameters (p);
    comp.prepare (fs);

    const int n = (int) fs * 3;
    std::vector<float> l ((size_t) n, CompressorDSP::dbToGain (inputDb));
    std::vector<float> r = l;
    float* chans[] = { l.data(), r.data() };
    comp.process (chans, 2, n);
    return CompressorDSP::gainToDb (l.back());
}

int main()
{
    // 정적 커브
    expectNear ("below threshold passes",     CompressorDSP::computeOutputLevelDb (-30.0f, -20.0f, 4.0f, 0.0f), -30.0f, 1e-4f);
    expectNear ("hard knee 4:1 above",        CompressorDSP::computeOutputLevelDb (-8.0f,  -20.0f, 4.0f, 0.0f), -17.0f, 1e-4f);
    expectNear ("soft knee at threshold",     CompressorDSP::computeOutputLevelDb (-20.0f, -20.0f, 4.0f, 6.0f), -20.0f - 0.75f * 9.0f / 12.0f, 1e-4f);
    expectNear ("soft knee continuous (low)", CompressorDSP::computeOutputLevelDb (-23.0f, -20.0f, 4.0f, 6.0f), -23.0f, 1e-4f);
    expectNear ("soft knee continuous (high)",CompressorDSP::computeOutputLevelDb (-17.0f, -20.0f, 4.0f, 6.0f), -20.0f + 3.0f / 4.0f, 1e-4f);
    expectNear ("ratio 1 is transparent",     CompressorDSP::computeOutputLevelDb (0.0f,   -20.0f, 1.0f, 6.0f), 0.0f, 1e-4f);

    // 정상상태 처리
    CompressorDSP::Parameters p;
    p.thresholdDb = -20.0f; p.ratio = 4.0f; p.kneeDb = 0.0f;
    p.attackMs = 5.0f; p.releaseMs = 50.0f; p.makeupDb = 0.0f; p.mix = 1.0f;

    expectNear ("steady state -8 dB in",  steadyStateOutputDb (p, -8.0f),  -17.0f, 0.05f);
    expectNear ("steady state -30 dB in", steadyStateOutputDb (p, -30.0f), -30.0f, 0.05f);

    p.makeupDb = 5.0f;
    expectNear ("makeup gain",            steadyStateOutputDb (p, -8.0f),  -12.0f, 0.05f);

    p.makeupDb = 0.0f; p.mix = 0.0f;
    expectNear ("mix 0 is dry",           steadyStateOutputDb (p, -8.0f),  -8.0f,  0.05f);

    p.mix = 1.0f; p.rmsDetector = true;
    expectNear ("RMS detector (DC)",      steadyStateOutputDb (p, -8.0f),  -17.0f, 0.05f);

    // attack 타이밍: attack 시간 후 목표 GR 의 약 63% 도달
    {
        CompressorDSP comp;
        CompressorDSP::Parameters a = p;
        a.rmsDetector = false; a.attackMs = 10.0f;
        comp.setParameters (a);
        comp.prepare (48000.0);
        const int n = 480; // 10 ms
        std::vector<float> x ((size_t) n, CompressorDSP::dbToGain (-8.0f));
        float* chans[] = { x.data() };
        const float gr = comp.process (chans, 1, n);
        expectNear ("attack reaches ~63% at attack time", gr, 9.0f * 0.632f, 0.1f);
    }

    std::printf (failures == 0 ? "\nAll tests passed\n" : "\n%d test(s) failed\n", failures);
    return failures == 0 ? 0 : 1;
}
