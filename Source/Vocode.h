#include <JuceHeader.h>
#include <math.h>

using namespace juce;
using namespace std;

const int fftPowerOf2 = 16;
const int fftSize = 1 << fftPowerOf2;
typedef array<float, fftSize * 2> FFTArray;
const float maxCarrierRSqrt = 9.0;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, const int outLen, float *outPtr);
static void getFFT(const float *samplePtr, int sampleCount, FFTArray& fftData);
static void getMagnitudes(FFTArray& fftData, array<float, fftSize>& binMagnitudes);
static void printSamples(const char *arrayName, float *array, int arraySize);
static void printRange(const char *arrayName, int lowerBound, int upperBound, float *array);

static void vocode(AudioBuffer<float>& carrierBuffer, AudioBuffer<float>& infoBuffer, AudioBuffer<float>& outBuffer) {
  int channelCount = carrierBuffer.getNumChannels();
  int infoBufferChannelCount = infoBuffer.getNumChannels();
  if (infoBufferChannelCount < channelCount) {
    channelCount = infoBufferChannelCount;
  }
  for (int ch = 0; ch < channelCount; ++ch) {
    const float *carrierPtr = carrierBuffer.getReadPointer(ch);
    const float *infoPtr = infoBuffer.getReadPointer(ch);
    float *outPtr = outBuffer.getWritePointer(ch);
    vocodeChannel(carrierPtr, infoPtr, outBuffer.getNumSamples(), outPtr);
  }
}

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, int outLen, float *outPtr) {
  // Some dummy buffer writing.
  //for (int i = 0; i < outLen; ++i) {
    //outPtr[i] = i % 2 == 0 ? carrierPtr[i] : infoPtr[i];
  //}

  // Run a real-only FFT on both signals.
  FFTArray carrierFFTData;
  FFTArray infoFFTData;
  getFFT(carrierPtr, outLen, carrierFFTData);
  getFFT(infoPtr, outLen, infoFFTData);

  // Get the magnitudes of the FFT bins.
  array<float, fftSize> carrierBinMagnitudes;
  array<float, fftSize> infoBinMagnitudes;
  getMagnitudes(carrierFFTData, carrierBinMagnitudes);
  getMagnitudes(infoFFTData, infoBinMagnitudes);

  // Get the reciprocal square roots of the magnitudes.
  const auto reciprocalSqRt = [](float bin){ return 1.0 / sqrt(bin); };
  array<float, fftSize> carrierBinMagnitudeReciprocalSqRts;
  array<float, fftSize> infoBinMagnitudeReciprocalSqRts;
  transform(carrierBinMagnitudes.begin(), carrierBinMagnitudes.end(),
    carrierBinMagnitudeReciprocalSqRts.begin(), reciprocalSqRt);
  transform(infoBinMagnitudes.begin(), infoBinMagnitudes.end(),
    infoBinMagnitudeReciprocalSqRts.begin(), reciprocalSqRt);
  // The fifth pair in carrierFFTData aligns with the fifth single value
  // in the magnitude array.
  printRange("carrierBinMagnitudeReciprocalSqRts", 5, 15, carrierBinMagnitudeReciprocalSqRts.data());
  printRange("infoBinMagnitudeReciprocalSqRts", 5, 15, infoBinMagnitudeReciprocalSqRts.data());

  // Clamp the carrier rsqrts. to a max value.
  const auto clamp = [](float val){ return val > maxCarrierRSqrt ? maxCarrierRSqrt : val; };
  array<float, fftSize> carrierBinMagRSqRtsClamped;
  transform(carrierBinMagnitudeReciprocalSqRts.begin(), carrierBinMagnitudeReciprocalSqRts.end(),
    carrierBinMagRSqRtsClamped.begin(), clamp);
  printRange("carrierBinMagRSqRtsClamped", 5, 15, carrierBinMagRSqRtsClamped.data());

  // Multiply the clamped carrier mag. rsqrts by the info carrier mag. sqrts.
  // Should probably do this in-place for perf. Maybe later.
  array<float, fftSize> combinedBinMagSqRts;
  FloatVectorOperations::multiply(combinedBinMagSqRts.data(),
    carrierBinMagRSqRtsClamped.data(), infoBinMagnitudeReciprocalSqRts.data(),
    fftSize);
  printRange("combinedBinMagSqRts", 5, 15, combinedBinMagSqRts.data());
}

// fftData will have real and imaginary parts interleaved.
static void getFFT(const float *samplePtr, int sampleCount, FFTArray& fftData) {
  fill(fftData.begin(), fftData.end(), 0.0f);
  const int sampleLimit = sampleCount > fftSize ? fftSize : sampleCount;
  for (int sampleIndex = 0; sampleIndex < sampleLimit; ++sampleIndex) {
    fftData[sampleIndex] = samplePtr[sampleIndex];
  }
  printSamples("fftData, before anything", fftData.data(), sampleLimit);

  dsp::WindowingFunction<float> window(fftSize, dsp::WindowingFunction<float>::hann);
  window.multiplyWithWindowingTable(fftData.data(), fftSize);

  printSamples("fftData, after windowing", fftData.data(), sampleLimit);

  dsp::FFT fft(fftPowerOf2);
  fft.performRealOnlyForwardTransform(fftData.data());

  printSamples("fftData, after FFT", fftData.data(), sampleLimit);
}

// Assumes fftData will have real and imaginary parts interleaved.
static void getMagnitudes(FFTArray& fftData, array<float, fftSize>& binMagnitudes) {
  for (int i = 0; i < fftData.size(); i += 2) {
    const float realSquared = pow(fftData[i], 2);
    const float imagSquared = pow(fftData[i + 1], 2);
    binMagnitudes[i/2] = sqrt(realSquared + imagSquared);
    if (i >= 10 && i < 21) {
      cout << fftData[i] << " realSquared: " << realSquared << fftData[i + 1] << " imagSquared: " << imagSquared << ", magnitude: " << binMagnitudes[i/2] << endl;
    }
  }
}

static void printSamples(const char *arrayName, float *array, int arraySize) {
  cout << arrayName << " example values early: " << array[arraySize/2] << ", " << array[arraySize/2+1] << endl;
  //cout << arrayName << " sample late: " << array[arraySize + arraySize/2] << endl;
  cout << arrayName << " example values late: " << array[arraySize + arraySize/2 + 2] << ", " << array[arraySize + arraySize/2 + 2] << endl;
}

static void printRange(const char *arrayName, int lowerBound, int upperBound, float *array) {
  cout << arrayName << " values ";
  for (int i = lowerBound; i < upperBound; ++i) {
    cout << "Value " << i << ": " << array[i] << ", ";
  }
  cout << endl;
}
