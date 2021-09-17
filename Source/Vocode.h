#include <JuceHeader.h>
#include <math.h>
#include "Utils.h"
#include "Consts.h"

using namespace juce;
using namespace std;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, const int outLen, float *outPtr);
static void vocodeBlock(const float *carrierPtr,const float *infoPtr, float *outPtr, int outLen);

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

  // We need overlap between blocks.
  for (int i = 0; i < outLen; i += floor(fftSize/overlapAmount)) {
    int end = i + fftSize;
    if (end > outLen) {
      end = outLen;
    }
    FFTArray carrierBlockArray;
    FFTArray infoBlockArray;
    FFTArray outBlockArray;

    copy(carrierPtr + i, carrierPtr + i + fftSize, carrierBlockArray.begin());
    copy(infoPtr + i, infoPtr + i + fftSize, infoBlockArray.begin());

    vocodeBlock(carrierBlockArray.data(), infoBlockArray.data(), outBlockArray.data(), fftSize);
    for (int j = i; j < end; ++j) {
      outPtr[j] += outBlockArray[j - i];
    }
  }
}

static void vocodeBlock(const float *carrierPtr,const float *infoPtr, float *outPtr, int outLen) {
  // Run a real-only FFT on both signals.
  ComplexFFTArray carrierFFTData;
  ComplexFFTArray infoFFTData;
  getFFT(carrierPtr, outLen, carrierFFTData);
  getFFT(infoPtr, outLen, infoFFTData);

  // Get the magnitudes of the FFT bins.
  FFTArray carrierBinMagnitudes;
  FFTArray infoBinMagnitudes;
  getMagnitudes(carrierFFTData, carrierBinMagnitudes);
  getMagnitudes(infoFFTData, infoBinMagnitudes);

  // Clamp the carrier magnitudes. to a max value.
  const auto clamp = [](float val){ return val > maxCarrierMag ? maxCarrierMag : val; };
  FFTArray carrierBinMagsClamped;
  transform(carrierBinMagnitudes.begin(), carrierBinMagnitudes.end(),
    carrierBinMagsClamped.begin(), clamp);
  printRange("carrierBinMagsClamped", 5, 15, carrierBinMagsClamped.data());

  // Multiply the clamped carrier mags by the info carrier mags.
  // Should probably do this in-place for perf. Maybe later.
  FFTArray combinedBinMags;
  FloatVectorOperations::multiply(combinedBinMags.data(),
    carrierBinMagsClamped.data(), infoBinMagnitudes.data(),
    fftSize);
  printRange("combinedBinMags", 5, 15, combinedBinMags.data());

  // Reduce the combined mags.
  FloatVectorOperations::multiply(
    combinedBinMags.data(), smallifyFactor, fftSize);
  printRange("combinedBinMags after reduction", 5, 15, combinedBinMags.data());

  // Combine the imaginary components of the carrier fft
  // with the reduced combined mags.
  FFTArray carrierImagBins;
  getImaginary(carrierFFTData, carrierImagBins);
  FFTArray carrierImagXReducedMagStuff;
  FloatVectorOperations::multiply(carrierImagXReducedMagStuff.data(),
    carrierImagBins.data(), combinedBinMags.data(),
    fftSize);

  // Reduce the real components of the carrier fft.
  FFTArray carrierRealBins;
  getReal(carrierFFTData, carrierRealBins);
  FFTArray carrierReducedRealBins;
  FloatVectorOperations::multiply(
    carrierReducedRealBins.data(), carrierRealBins.data(),
    smallifyFactor, fftSize);

  ComplexFFTArray ifftData;
  getIFFT(carrierReducedRealBins, combinedBinMags, ifftData);

  // Copy the results to the channel.
  const int sampleLimit = outLen > fftSize ? fftSize : outLen;
  for (int i = 0; i < sampleLimit; ++i) {
    outPtr[i] = ifftData[i];
  }
}
