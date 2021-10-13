#include <JuceHeader.h>
#include <math.h>
#include "Utils.h"
#include "Consts.h"
#include "LogSignal.h"

using namespace juce;
using namespace std;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, int outLen, DebugSignals& debugSignals, float *outPtr);
//static void vocodeChannel(const float *carrierPtr, const float *infoPtr, int outLen, float *outPtr);
static void vocodeBlock(
  const float *carrierPtr,
  const float *infoPtr,
  // offsetOfBlock is how far into the whole signal that this
  // particular block starts.
  int offsetOfBlock,
  DebugSignals& debugSignals,
  float *outPtr,
  int outLen);
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

    vocodeBlock(
      carrierBlockArray.data(),
      infoBlockArray.data(),
      i,
      debugSignals,
      outBlockArray.data(),
      end - i);

    // TODO: Have vocodeBlock work directly on outPtr.
    for (int j = i; j < end; ++j) {
      outPtr[j] += outBlockArray[j - i];
    }
  }
}

static void vocodeBlock(
  const float *carrierPtr,
  const float *infoPtr,
  // offsetOfBlock is how far into the whole signal that this
  // particular block starts.
  int offsetOfBlock,
  DebugSignals& debugSignals,
  float *outPtr,
  int outLen) {

  // Run a real-only FFT on both signals.
  ComplexFFTArray carrierFFTData;
  ComplexFFTArray infoFFTData;

  for (int sampleIndex = 0; sampleIndex < outLen; ++sampleIndex) {
    carrierFFTData[sampleIndex] = carrierPtr[sampleIndex];
    infoFFTData[sampleIndex] = infoPtr[sampleIndex];
  }

  logSignal("carrier-raw.txt", outLen, carrierPtr);
  logSignal("carrier.txt", outLen, carrierFFTData.data());

  applyHannWindow(carrierFFTData);
  applyHannWindow(infoFFTData);

  saveArrayToDebug(carrierFFTData.data(), offsetOfBlock, outLen, "carrierHann", debugSignals);
  // TODO: Include channel in filename.
  logSignal("carrierHann.txt", outLen, carrierFFTData.data());
  saveArrayToDebug(infoFFTData.data(), offsetOfBlock, outLen, "infoHann", debugSignals);

  getFFT(carrierFFTData);
  getFFT(infoFFTData);

  saveArrayToDebug(carrierFFTData.data(), offsetOfBlock, outLen, "carrierFFT", debugSignals);
  saveArrayToDebug(infoFFTData.data(), offsetOfBlock, outLen, "infoFFT", debugSignals);

  squareSignal(carrierFFTData.data(), fftSize * 2);
  squareSignal(infoFFTData.data(), fftSize * 2);
  FFTArray carrierFFTSqAdded;
  FFTArray infoFFTSqAdded;
  addRealAndImag(carrierFFTData, carrierFFTSqAdded);
  addRealAndImag(infoFFTData, infoFFTSqAdded);

  saveArrayToDebug(carrierFFTSqAdded.data(), offsetOfBlock, outLen, "carrierFFTSqAdded", debugSignals);
  saveArrayToDebug(infoFFTSqAdded.data(), offsetOfBlock, outLen, "infoFFTSqAdded", debugSignals);

  FFTArray carrierFFTSqAddedRSqrt;
  FFTArray infoFFTSqAddedSqrt;
  rSqrtSignal(carrierFFTSqAdded.data(), fftSize, carrierFFTSqAddedRSqrt.data());
  sqrtSignal(infoFFTSqAdded.data(), fftSize, infoFFTSqAddedSqrt.data());

  saveArrayToDebug(carrierFFTSqAddedRSqrt.data(), offsetOfBlock, outLen, "carrierFFTSqAddedRSqrt", debugSignals);
  saveArrayToDebug(infoFFTSqAddedSqrt.data(), offsetOfBlock, outLen, "infoFFTSqAddedSqrt", debugSignals);

  FFTArray combinedAmpFactors;
  FloatVectorOperations::multiply(
    combinedAmpFactors.data(), // dest
    carrierFFTSqAddedRSqrt.data(),
    infoFFTSqAddedSqrt.data(),
    fftSize);
  printRange("combinedAmpFactors", 5, 15, combinedAmpFactors.data());

  // Turn down the combined amps.
  FFTArray reducedAmpFactors;
  FloatVectorOperations::multiply(
    reducedAmpFactors.data(), combinedAmpFactors.data(), smallifyFactor, fftSize);
  printRange("reducedAmpFactors after reduction", 5, 15, reducedAmpFactors.data());

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

  // Multiply the imaginary components of the carrier fft by the reduced
  // combined amps.
  FFTArray carrierImagBins;
  getImaginary(carrierFFTData, carrierImagBins);
  FFTArray carrierImagWithReducedAmpFactors;
  FloatVectorOperations::multiply(
    carrierImagWithReducedAmpFactors.data(),
    carrierImagBins.data(),
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
