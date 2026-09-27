# TMRX Compressor

JUCE 로 만든 컴프레서 오디오 플러그인입니다. **VST3 / AAX / AU / Standalone** 포맷을 지원합니다.

## 기능

| 파라미터 | 범위 | 설명 |
|---|---|---|
| Threshold | -60 ~ 0 dB | 컴프레션이 시작되는 레벨 |
| Ratio | 1:1 ~ 20:1 | 압축 비율 |
| Knee | 0 ~ 24 dB | 소프트 니 폭 (0 = 하드 니) |
| Attack | 0.1 ~ 200 ms | 게인 리덕션이 걸리는 속도 |
| Release | 5 ~ 2000 ms | 게인 리덕션이 풀리는 속도 |
| Makeup | -12 ~ +24 dB | 출력 보정 게인 |
| Mix | 0 ~ 100 % | Dry/Wet (패럴렐 컴프레션) |
| RMS | on/off | Peak / RMS 디텍터 선택 |

- 스테레오 링크 피드포워드 디자인 (좌우 채널에 동일한 게인 적용 → 스테레오 이미지 유지)
- 로그(dB) 도메인 attack/release 스무딩, 소프트 니
- 레이턴시 0, 모노/스테레오 지원
- 게인 리덕션(GR) 미터

## 프로젝트 구조

```
CMakeLists.txt              빌드 설정 (JUCE 자동 다운로드)
Source/dsp/CompressorDSP.h  컴프레서 DSP 코어 (JUCE 비의존, 테스트 가능)
Source/PluginProcessor.*    파라미터 / 오디오 처리 / 상태 저장
Source/PluginEditor.*       UI (노브 7개 + RMS 토글 + GR 미터)
Tests/CompressorTests.cpp   DSP 단위 테스트
```

## 빌드

필요: CMake 3.22+, C++17 컴파일러 (Windows: Visual Studio 2022, macOS: Xcode, Linux: GCC/Clang)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release          # DSP 테스트
```

JUCE 는 처음 configure 할 때 자동으로 받아옵니다. 이미 받아둔 JUCE 가 있다면 `-DJUCE_DIR=/path/to/JUCE` 로 지정하세요.

결과물 위치: `build/TMRXCompressor_artefacts/Release/VST3/TMRX Compressor.vst3`

빌드 후 DAW 플러그인 폴더로 자동 복사하려면 `-DTMRX_COPY_PLUGIN_AFTER_BUILD=ON` 을 추가하세요.

## AAX (Pro Tools) 빌드

AAX 는 Avid 정책상 SDK 를 저장소에 포함할 수 없어서 추가 절차가 필요합니다.

1. [Avid Developer](https://developer.avid.com/) 계정을 만들고 **AAX SDK** 를 다운로드합니다.
2. SDK 경로를 지정해서 configure 합니다.
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAAX_SDK_PATH=/path/to/aax-sdk
   cmake --build build --config Release --target TMRXCompressor_AAX
   ```
3. 개발 중 테스트는 **Pro Tools Developer** 버전(Avid 에서 제공)에서 서명 없이 로드할 수 있습니다.
4. 일반 Pro Tools 에서 쓰거나 배포하려면 **PACE Eden 툴 (wraptool)** 로 서명해야 합니다.
   Avid 개발자 프로그램을 통해 PACE 계정을 신청하세요.

AAX 는 Windows / macOS 에서만 빌드됩니다.

## 배포 시 참고

- **macOS**: 배포하려면 Apple Developer ID 로 코드 서명 + 공증(notarization) 이 필요합니다.
- **VST3**: Steinberg VST3 SDK 는 MIT 라이선스라 별도 계약 없이 배포 가능합니다.
- **JUCE**: JUCE 8 은 AGPLv3 / 상용 라이선스 듀얼입니다. 소스를 공개하지 않는 상용 배포라면 JUCE 라이선스(Starter 는 매출 조건부 무료)를 확인하세요.
- `PLUGIN_MANUFACTURER_CODE` (`Tmrx`) / `PLUGIN_CODE` (`Tcmp`) 는 한 번 배포하면 바꾸지 마세요. 바꾸면 DAW 가 다른 플러그인으로 인식해서 기존 세션이 깨집니다.
