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
  for (int i = 0; i < outLen; i += floor(fftSize/3)) {
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

  // Get the reciprocal square roots of the magnitudes.
  const auto reciprocalSqRt = [](float bin){ return 1.0 / sqrt(bin); };
  FFTArray carrierBinMagnitudeReciprocalSqRts;
  FFTArray infoBinMagnitudeReciprocalSqRts;
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
  FFTArray carrierBinMagRSqRtsClamped;
  transform(carrierBinMagnitudeReciprocalSqRts.begin(), carrierBinMagnitudeReciprocalSqRts.end(),
    carrierBinMagRSqRtsClamped.begin(), clamp);
  printRange("carrierBinMagRSqRtsClamped", 5, 15, carrierBinMagRSqRtsClamped.data());

  // Multiply the clamped carrier mag. rsqrts by the info carrier mag. sqrts.
  // Should probably do this in-place for perf. Maybe later.
  FFTArray combinedBinMagRSqRts;
  FloatVectorOperations::multiply(combinedBinMagRSqRts.data(),
    carrierBinMagRSqRtsClamped.data(), infoBinMagnitudeReciprocalSqRts.data(),
    fftSize);
  printRange("combinedBinMagRSqRts", 5, 15, combinedBinMagRSqRts.data());

  // Reduce the combined mag. rsqrts.
  FloatVectorOperations::multiply(
    combinedBinMagRSqRts.data(), smallifyFactor, fftSize);
  printRange("combinedBinMagRSqRts after reduction", 5, 15, combinedBinMagRSqRts.data());

  // Combine the imaginary components of the carrier fft
  // with the reduced combined mag. rsqrts.
  FFTArray carrierImagBins;
  getImaginary(carrierFFTData, carrierImagBins);
  FFTArray carrierImagXReducedMagStuff;
  FloatVectorOperations::multiply(carrierImagXReducedMagStuff.data(),
    carrierImagBins.data(), combinedBinMagRSqRts.data(),
    fftSize);

  // Reduce the real components of the carrier fft.
  FFTArray carrierRealBins;
  getReal(carrierFFTData, carrierRealBins);
  FFTArray carrierReducedRealBins;
  FloatVectorOperations::multiply(
    carrierRealBins.data(), smallifyFactor, fftSize);

  ComplexFFTArray ifftData;
  getIFFT(carrierReducedRealBins, combinedBinMagRSqRts, ifftData);

  // Copy the results to the channel.
  const int sampleLimit = outLen > fftSize ? fftSize : outLen;
  for (int i = 0; i < sampleLimit; ++i) {
    outPtr[i] = ifftData[i];
  }
}
