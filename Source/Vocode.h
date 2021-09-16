#include <JuceHeader.h>

using namespace juce;

const int fftPowerOf2 = 16;
const int fftSize = 1 << fftPowerOf2;
typedef std::array<float, fftSize * 2> FFTArray;

static void vocodeChannel(float *carrierPtr, float *infoPtr, const int outLen, float *outPtr);
static void getFFT(float *samplePtr, int sampleCount, FFTArray& fftData);

static void vocode(AudioBuffer<float>& carrierBuffer, AudioBuffer<float>& infoBuffer, AudioBuffer<float>& outBuffer) {
  int channelCount = carrierBuffer.getNumChannels();
  int infoBufferChannelCount = infoBuffer.getNumChannels();
  if (infoBufferChannelCount < channelCount) {
    channelCount = infoBufferChannelCount;
  }
  for (int ch = 0; ch < channelCount; ++ch) {
    float *carrierPtr = carrierBuffer.getWritePointer(ch);
    float *infoPtr = infoBuffer.getWritePointer(ch);
    float *outPtr = outBuffer.getWritePointer(ch);
    vocodeChannel(carrierPtr, infoPtr, outBuffer.getNumSamples(), outPtr);
  }
}

static void vocodeChannel(float *carrierPtr, float *infoPtr, int outLen, float *outPtr) {
  // Some dummy buffer writing.
  //for (int i = 0; i < outLen; ++i) {
    //outPtr[i] = i % 2 == 0 ? carrierPtr[i] : infoPtr[i];
  //}
  FFTArray carrierFFTData;
  FFTArray infoFFTData;
  getFFT(carrierPtr, outLen, carrierFFTData);
  getFFT(infoPtr, outLen, infoFFTData);
}

static void getFFT(float *samplePtr, int sampleCount, FFTArray& fftData) {
  std::fill(fftData.begin(), fftData.end(), 0.0f);
  const int sampleLimit = sampleCount > fftSize ? fftSize : sampleCount;
  for (int sampleIndex = 0; sampleIndex < sampleLimit; ++sampleIndex) {
    fftData[sampleIndex] = samplePtr[sampleIndex];
  }
  std::cout << "fftData sample early: " << fftData[sampleLimit/2] << std::endl;
  std::cout << "fftData sample late: " << fftData[sampleLimit + sampleLimit/2] << std::endl;
  dsp::WindowingFunction<float> window(fftSize, dsp::WindowingFunction<float>::hann);
  dsp::FFT fft(fftPowerOf2);
}
