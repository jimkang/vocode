#include <JuceHeader.h>
#include <math.h>
#include "Utils.h"
#include "Consts.h"

using namespace juce;
using namespace std;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, int outLen, DebugSignals& debugSignals, float *outPtr);
//static void vocodeChannel(const float *carrierPtr, const float *infoPtr, int outLen, float *outPtr);
static void vocodeBlock(const float *carrierPtr, const float *infoPtr, DebugSignals& debugSignals, float *outPtr, int outLen);
//static void vocodeBlock(const float *carrierPtr, const float *infoPtr, float *outPtr, int outLen);
static void getReducedCombinedAmpFactors(
  ComplexFFTArray& carrierFFTData, ComplexFFTArray& infoFFTData, FFTArray& reducedAmpFactors);

static void vocode(AudioBuffer<float>& carrierBuffer, AudioBuffer<float>& infoBuffer,
  DebugSignalsForChannels& debugSignalsForChannels,
  AudioBuffer<float>& outBuffer) {

  int channelCount = carrierBuffer.getNumChannels();
  int infoBufferChannelCount = infoBuffer.getNumChannels();
  if (infoBufferChannelCount < channelCount) {
    channelCount = infoBufferChannelCount;
  }
  for (int ch = 0; ch < channelCount; ++ch) {
    const float *carrierPtr = carrierBuffer.getReadPointer(ch);
    const float *infoPtr = infoBuffer.getReadPointer(ch);
    float *outPtr = outBuffer.getWritePointer(ch);

    //DebugSignal debugSignals;
//
    //for (auto it = debugSignals.begin();
      //it != debugSignals.end();
      //++it) {
      //auto audioBuffer = it->second;
      //debugSignals[it->first] = audioBuffer.getWritePointer(ch);
    //}


    vocodeChannel(carrierPtr, infoPtr, outBuffer.getNumSamples(), *debugSignalsForChannels[ch], outPtr);
  }
}

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, int outLen, DebugSignals& debugSignals, float *outPtr) {
  // Some dummy buffer writing.
  //for (int i = 0; i < outLen; ++i) {
    //outPtr[i] = i % 2 == 0 ? carrierPtr[i] : infoPtr[i];
  //}

  // We need overlap between blocks.
  for (int i = 0; i < outLen; i += floor(fftSize - overlapSize)) {
    int end = i + fftSize;
    if (end > outLen) {
      end = outLen;
    }
    FFTArray carrierBlockArray;
    FFTArray infoBlockArray;
    FFTArray outBlockArray;

    copy(carrierPtr + i, carrierPtr + i + fftSize, carrierBlockArray.begin());
    copy(infoPtr + i, infoPtr + i + fftSize, infoBlockArray.begin());

    //printRange("carrierPtr", i, i + fftSize, carrierPtr);
    //printRange("carrierBlockArray", 0, fftSize, carrierBlockArray.data());

    cout << "vocoding block starting at " << i << endl;

    vocodeBlock(carrierBlockArray.data(), infoBlockArray.data(), debugSignals,
outBlockArray.data(), end - i);
    for (int j = i; j < end; ++j) {
      outPtr[j] += outBlockArray[j - i];
    }
  }
}

static void vocodeBlock(const float *carrierPtr, const float *infoPtr,
DebugSignals& debugSignals,
 float *outPtr, int outLen) {
  // Run a real-only FFT on both signals.
  ComplexFFTArray carrierFFTData;
  ComplexFFTArray infoFFTData;
  fill(carrierFFTData.begin(), carrierFFTData.end(), 0.0f);
  fill(infoFFTData.begin(), infoFFTData.end(), 0.0f);

  applyHannWindow(carrierFFTData);
  applyHannWindow(infoFFTData);

  float *signal = debugSignals["carrierHann"];

  writeArrayToPtr(carrierFFTData, "carrierHann", debugSignals);
  writeArrayToPtr(infoFFTData, "infoHann", debugSignals);

  getFFT(carrierPtr, outLen, carrierFFTData);
  getFFT(infoPtr, outLen, infoFFTData);

  FFTArray reducedAmpFactors;
  getReducedCombinedAmpFactors(carrierFFTData, infoFFTData, reducedAmpFactors);

  // Combine the imaginary components of the carrier fft
  // with the reduced combined amps.
  FFTArray carrierImagBins;
  getImaginary(carrierFFTData, carrierImagBins);
  FFTArray carrierImagWithReducedAmpFactors;
  FloatVectorOperations::multiply(
    carrierImagWithReducedAmpFactors.data(),
    carrierImagBins.data(),
    reducedAmpFactors.data(),
    fftSize);

  // Multiply the real components of the carrier fft by the reduced
  // combined amps.
  FFTArray carrierRealBins;
  getReal(carrierFFTData, carrierRealBins);
  FFTArray carrierRealWithReducedAmpFactors;
  FloatVectorOperations::multiply(
    carrierRealWithReducedAmpFactors.data(),
    carrierRealBins.data(),
    reducedAmpFactors.data(),
    fftSize);

  ComplexFFTArray ifftData;
  getIFFT(carrierRealWithReducedAmpFactors, carrierImagWithReducedAmpFactors, ifftData);
  //getIFFT(carrierRealBins, carrierImagBins, ifftData);

  // Copy the results to the channel.
  const int sampleLimit = outLen > fftSize ? fftSize : outLen;
  for (int i = 0; i < sampleLimit; ++i) {
    outPtr[i] = ifftData[i];
  }
}

static void getReducedCombinedAmpFactors(
  ComplexFFTArray& carrierFFTData, ComplexFFTArray& infoFFTData, FFTArray& reducedAmpFactors) {

  // Get the magnitudes of the FFT bins.
  FFTArray carrierAmpFactors;
  FFTArray infoAmpFactors;
  getMagnitudes(carrierFFTData, carrierAmpFactors, false);
  getMagnitudes(infoFFTData, infoAmpFactors, false);

  // Clamp the carrier magnitudes. to a max value.
  const auto clamp = [](float val){ return val > maxCarrierMag ? maxCarrierMag : val; };
  FFTArray carrierAmpFactorsClamped;
  transform(carrierAmpFactors.begin(), carrierAmpFactors.end(),
    carrierAmpFactorsClamped.begin(), clamp);
  printRange("carrierAmpFactorsClamped", 5, 15, carrierAmpFactorsClamped.data());

  // Multiply the clamped carrier amps by the info carrier amps.
  FFTArray combinedAmpFactors;
  FloatVectorOperations::multiply(
    combinedAmpFactors.data(),
    carrierAmpFactorsClamped.data(),
    //carrierAmpFactors.data(),
    infoAmpFactors.data(),
    fftSize);
  printRange("combinedAmpFactors", 5, 15, combinedAmpFactors.data());

  // Reduce the combined amps.
  FloatVectorOperations::multiply(
    reducedAmpFactors.data(), combinedAmpFactors.data(), smallifyFactor, fftSize);
  printRange("reducedAmpFactors after reduction", 5, 15, reducedAmpFactors.data());
}
