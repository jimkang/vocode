#include <JuceHeader.h>
#include <math.h>

using namespace juce;

const int fftPowerOf2 = 16;
const int fftSize = 1 << fftPowerOf2;
typedef std::array<float, fftSize * 2> FFTArray;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, const int outLen, float *outPtr);
static void getFFT(const float *samplePtr, int sampleCount, FFTArray& fftData);
static void getMagnitudes(FFTArray& fftData, std::array<float, fftSize>& binMagnitudes);
static void printSamples(const char *arrayName, float *array, int arraySize);

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

  std::array<float, fftSize> carrierBinMagnitudes;
  std::array<float, fftSize> infoBinMagnitudes;
  getMagnitudes(carrierFFTData, carrierBinMagnitudes);
  getMagnitudes(infoFFTData, infoBinMagnitudes);
}

// fftData will have real and imaginary parts interleaved.
static void getFFT(const float *samplePtr, int sampleCount, FFTArray& fftData) {
  std::fill(fftData.begin(), fftData.end(), 0.0f);
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
static void getMagnitudes(FFTArray& fftData, std::array<float, fftSize>& binMagnitudes) {
  for (int i = 0; i < fftData.size(); i += 2) {
    const float realSquared = pow(fftData[i], 2);
    const float imagSquared = pow(fftData[i + 1], 2);
    binMagnitudes[i/2] = sqrt(realSquared + imagSquared);
    if (i > 10 && i < 21) {
      std::cout << fftData[i] << " realSquared: " << realSquared << fftData[i + 1] << " imagSquared: " << imagSquared << ", magnitude: " << binMagnitudes[i/2] << std::endl;
    }
  }
}

static void printSamples(const char *arrayName, float *array, int arraySize) {
  std::cout << arrayName << " example values early: " << array[arraySize/2] << ", " << array[arraySize/2+1] << std::endl;
  //std::cout << arrayName << " sample late: " << array[arraySize + arraySize/2] << std::endl;
  std::cout << arrayName << " example values late: " << array[arraySize + arraySize/2 + 2] << ", " << array[arraySize + arraySize/2 + 2] << std::endl;
}
