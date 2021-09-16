#include <JuceHeader.h>

using namespace juce;

const int fftPowerOf2 = 16;
const int fftSize = 1 << fftPowerOf2;
typedef std::array<float, fftSize * 2> FFTArray;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, const int outLen, float *outPtr);
static void getFFT(const float *samplePtr, int sampleCount, FFTArray& fftData);
static void printSamples(const char *arrayName, FFTArray& fftData, int arraySize);

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
  FFTArray carrierFFTData;
  FFTArray infoFFTData;
  getFFT(carrierPtr, outLen, carrierFFTData);
  getFFT(infoPtr, outLen, infoFFTData);
}

static void getFFT(const float *samplePtr, int sampleCount, FFTArray& fftData) {
  std::fill(fftData.begin(), fftData.end(), 0.0f);
  const int sampleLimit = sampleCount > fftSize ? fftSize : sampleCount;
  for (int sampleIndex = 0; sampleIndex < sampleLimit; ++sampleIndex) {
    fftData[sampleIndex] = samplePtr[sampleIndex];
  }
  printSamples("fftData, before FFT", fftData, sampleLimit);

  dsp::WindowingFunction<float> window(fftSize, dsp::WindowingFunction<float>::hann);
  dsp::FFT fft(fftPowerOf2);
  fft.performRealOnlyForwardTransform(fftData.data());

  printSamples("fftData, after FFT", fftData, sampleLimit);
}

static void printSamples(const char *arrayName, FFTArray& fftData, int arraySize) {
  std::cout << arrayName << " sample early: " << fftData[arraySize/2] << std::endl;
  //std::cout << arrayName << " sample late: " << fftData[arraySize + arraySize/2] << std::endl;
  std::cout << arrayName << " sample late: " << fftData[arraySize + arraySize/2 + 1] << std::endl;
}
