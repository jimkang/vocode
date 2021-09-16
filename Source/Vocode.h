#include <JuceHeader.h>

using namespace juce;

const int fftPowerOf2 = 3;
const int fftSize = 1 << fftPowerOf2;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, const int outLen, float *outPtr);

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

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, const int outLen, float *outPtr) {
  // Some dummy buffer writing.
  for (int i = 0; i < outLen; ++i) {
    outPtr[i] = i % 2 == 0 ? carrierPtr[i] : infoPtr[i];
  }

  dsp::WindowingFunction<float> window(fftSize, dsp::WindowingFunction<float>::hann);
  dsp::FFT fft(fftPowerOf2);

  std::array<float, fftSize * 2> fftData;

}
