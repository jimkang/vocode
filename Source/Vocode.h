#include <JuceHeader.h>
#include <math.h>
#include "Utils.h"
#include "Consts.h"
#include "LogSignal.h"

using namespace juce;
using namespace std;

static void vocodeChannel(const vector<float>& carrierSamples, const vector<float>& infoSamples, vector<float>& outSamples, const DebugSignals& debugSignals);
static void vocodeBlock(const vector<float>& carrierBlockSamples, const vector<float>& infoBlockSamples, vector<float>& outBlockSamples, const DebugSignals& debugSignals);

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
    const int maxSamples = outBuffer.getNumSamples();
    // Copy the arrays into vectors.
    vector<float> carrierSamples(carrierBuffer.getReadPointer(ch), carrierBuffer.getReadPointer(ch) + maxSamples);
    vector<float> infoSamples(infoBuffer.getReadPointer(ch), infoBuffer.getReadPointer(ch) + maxSamples);
    vector<float> outSamples(maxSamples);

    //for (int i = 0; i < outSamples.size(); ++i) {
      //outSamples[i] = carrierSamples[i];
    //}

    vocodeChannel(carrierSamples, infoSamples, outSamples, *debugSignalsForChannels[ch]);

    //for (auto it = outSamples.begin(); it != outSamples.end(); ++it) {
      //if (*it > 0.01) {
        //cout << "hey";
      //}
    //}
    float *outWritePtr = outBuffer.getWritePointer(ch);
    for (int i = 0; i < outSamples.size(); ++i) {
      outWritePtr[i] = outSamples[i];
      if (outSamples[i] > 0) {
        int x = 2;
      }
    }
  }
}

static void vocodeChannel(const vector<float>& carrierSamples, const vector<float>& infoSamples, vector<float>& outSamples, const DebugSignals& debugSignals) {
  const int maxBlocks = outSamples.size()/fftSize;
  // Leave out the last partial block for now.

  auto carrierStart = carrierSamples.begin();
  auto infoStart = infoSamples.begin();
  auto outStart = outSamples.begin();

  //for (int i = 0; i < outSamples.size(); ++i) {
    //outSamples[i] = carrierSamples[i];
  //}
  //return;

  for (int blockIndex = 0; blockIndex < maxBlocks; ++blockIndex) {
    cout << "vocoding block  " << blockIndex << endl;

    auto carrierNext = carrierStart + blockSize;
    auto infoNext = infoStart + blockSize;
    auto outNext = outStart + blockSize;
    vector<float> outBlockSamples(outStart, outNext);

    vocodeBlock(
      vector<float>(carrierStart, carrierNext),
      vector<float>(infoStart, infoNext),
      outBlockSamples,
      debugSignals
    );

    // TODO: Cut down on the copying.
    int outBlockSampleIndex = 0;
    for (auto it = outStart; it!= outNext; ++it) {
      *it = outBlockSamples[outBlockSampleIndex];
      ++outBlockSampleIndex;
    }

    carrierStart = carrierNext;
    infoStart = infoNext;
    outStart = outNext;
  }
}

static void vocodeBlock(const vector<float>& carrierBlockSamples, const vector<float>& infoBlockSamples, vector<float>& outBlockSamples, const DebugSignals& debugSignals) {

  //auto carrierSample = carrierBlockSamples.begin();
  //auto outSample = outBlockSamples.begin();
  //for (int i = 0; i < outBlockSamples.size(); ++i) {
    //outBlockSamples[i] = carrierSample[i];
  //}
  //return;

  // Run a real-only FFT on both signals.
  ComplexFFTArray carrierFFTData;
  ComplexFFTArray infoFFTData;

  for (int i = 0; i < carrierBlockSamples.size(); ++i) {
    carrierFFTData[i] = carrierBlockSamples[i];
    infoFFTData[i] = infoBlockSamples[i];
  }

  logSignal("carrier-raw.txt", carrierBlockSamples.size(), carrierBlockSamples.data());
  logSignal("carrier.txt", fftSize, carrierFFTData.data());

  applyHannWindow(carrierFFTData);
  applyHannWindow(infoFFTData);

  // TODO: Include channel in filename.
  logSignal("carrierHann.txt", fftSize, carrierFFTData.data());

  getFFT(carrierFFTData);
  getFFT(infoFFTData);

  logSignal("carrierFFT.txt", fftSize, carrierFFTData.data());

  squareSignal(carrierFFTData.data(), fftSize * 2);
  squareSignal(infoFFTData.data(), fftSize * 2);
  FFTArray carrierFFTSqAdded;
  FFTArray infoFFTSqAdded;
  addRealAndImag(carrierFFTData, carrierFFTSqAdded);
  logSignal("carrier-rfft-added.txt", fftSize, carrierFFTSqAdded.data());
  addRealAndImag(infoFFTData, infoFFTSqAdded);

  //saveArrayToDebug(carrierFFTSqAdded.data(), offsetOfBlock, maxSamples, "carrierFFTSqAdded", debugSignals);
  //saveArrayToDebug(infoFFTSqAdded.data(), offsetOfBlock, maxSamples, "infoFFTSqAdded", debugSignals);

  FFTArray carrierFFTSqAddedRSqrt;
  FFTArray infoFFTSqAddedSqrt;
  rSqrtSignal(carrierFFTSqAdded.data(), fftSize, carrierFFTSqAddedRSqrt.data());
  logSignal("carrierRSqrt.txt", fftSize, carrierFFTSqAddedRSqrt.data());
  sqrtSignal(infoFFTSqAdded.data(), fftSize, infoFFTSqAddedSqrt.data());
  logSignal("infoSqrt.txt", fftSize, infoFFTSqAddedSqrt.data());

  //saveArrayToDebug(carrierFFTSqAddedRSqrt.data(), offsetOfBlock, maxSamples, "carrierFFTSqAddedRSqrt", debugSignals);
  //saveArrayToDebug(infoFFTSqAddedSqrt.data(), offsetOfBlock, maxSamples, "infoFFTSqAddedSqrt", debugSignals);

  FFTArray combinedAmpFactors;
  FloatVectorOperations::multiply(
    combinedAmpFactors.data(), // dest
    carrierFFTSqAddedRSqrt.data(),
    infoFFTSqAddedSqrt.data(),
    fftSize);
  printRange("combinedAmpFactors", 5, 15, combinedAmpFactors.data());
  logSignal("carrier-roots-multiplied.txt", fftSize, combinedAmpFactors.data());

  // Turn down the combined amps.
  FFTArray reducedAmpFactors;
  FloatVectorOperations::multiply(
    reducedAmpFactors.data(), combinedAmpFactors.data(), smallifyFactor, fftSize);
  printRange("reducedAmpFactors after reduction", 5, 15, reducedAmpFactors.data());

  // Multiply the reduced real components of the carrier fft by the reduced
  // combined amps.
  FFTArray carrierRealBins;
  getReal(carrierFFTData, carrierRealBins);
  FFTArray reducedCarrierReals;
  FloatVectorOperations::multiply(
    reducedCarrierReals.data(), carrierRealBins.data(), smallifyFactor, fftSize);
  logSignal("carrier-fft-real-reduced.txt", fftSize, reducedCarrierReals.data());

  FFTArray carrierRealWithReducedAmpFactors;
  FloatVectorOperations::multiply(
    carrierRealWithReducedAmpFactors.data(),
    reducedCarrierReals.data(),
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
  for (int i = 0; i < fftSize; ++i) {
    outBlockSamples[i] = ifftData[i];
  }
}
