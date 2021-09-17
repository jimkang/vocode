#include <JuceHeader.h>
#include <math.h>
#include "Utils.h"
#include "Consts.h"

using namespace juce;
using namespace std;

static void reconstructChannel(const float *carrierPtr, const int outLen, float *outPtr);
static void reconstructBlock(const float *carrierPtr, float *outPtr, int outLen);

static void reconstruct(AudioBuffer<float>& carrierBuffer, AudioBuffer<float>& outBuffer) {
  int channelCount = carrierBuffer.getNumChannels();

  for (int ch = 0; ch < channelCount; ++ch) {
    const float *carrierPtr = carrierBuffer.getReadPointer(ch);
    float *outPtr = outBuffer.getWritePointer(ch);
    reconstructChannel(carrierPtr, outBuffer.getNumSamples(), outPtr);
  }
}

static void reconstructChannel(const float *carrierPtr, int outLen, float *outPtr) {
  // We need overlap between blocks.
  for (int i = 0; i < outLen; i += floor(fftSize/3)) {
    int end = i + fftSize;
    if (end > outLen) {
      end = outLen;
    }
    FFTArray blockArray;
    copy(carrierPtr + i, carrierPtr + i + fftSize, blockArray.begin());
    FFTArray outBlockArray;
    reconstructBlock(blockArray.data(), outBlockArray.data(), fftSize);
    for (int j = i; j < end; ++j) {
      outPtr[j] += outBlockArray[j - i];
    }
  }
}

static void reconstructBlock(const float *carrierPtr, float *outPtr, int outLen) {
  // Run a real-only FFT on carrier signal
  ComplexFFTArray carrierFFTData;
  getFFT(carrierPtr, outLen, carrierFFTData);

  FFTArray carrierRealBins;
  getReal(carrierFFTData, carrierRealBins);
  FFTArray carrierImagBins;
  getImaginary(carrierFFTData, carrierImagBins);

  ComplexFFTArray testZipArray;
  zipTogetherComplexArray(carrierRealBins,
    carrierImagBins, testZipArray);

  cout << "Are carrierFFTData and testZipArray equal?" << (carrierFFTData == testZipArray) << endl;

  ComplexFFTArray ifftData;
  getIFFT(carrierRealBins, carrierImagBins, ifftData);

  // Copy the results to the channel.
  const int sampleLimit = outLen > fftSize ? fftSize : outLen;
  cout << "outLen: " << outLen << ", fftSize: " << fftSize << endl;
  for (int i = 0; i < sampleLimit; ++i) {
    outPtr[i] = ifftData[i];
    //outPtr[i] = carrierFFTData[i];
  }
}
