#include <JuceHeader.h>
#include <math.h>
#include "Utils.h"
#include "Consts.h"
#include "LogSignal.h"

using namespace juce;
using namespace std;

const int blockIndexToLog = 100;

static void vocodeChannel(vector<float>& carrierSamples, vector<float>& infoSamples, double sampleRate, vector<float>& outSamples);
static void vocodeBlock(vector<float>& carrierBlockSamples, vector<float>& infoBlockSamples, IIRFilter& carrierHighPassFilter, IIRFilter& infoHighPassFilter, vector<float>& outBlockSamples, int blockIndex);

static void getReducedCombinedAmpFactors(
  ComplexFFTArray& carrierFFTData, ComplexFFTArray& infoFFTData, FFTArray& reducedAmpFactors);

static void vocode(AudioBuffer<float>& carrierBuffer, AudioBuffer<float>& infoBuffer,
  double sampleRate,
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

    vocodeChannel(carrierSamples, infoSamples, sampleRate, outSamples);

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

static void vocodeChannel(vector<float>& carrierSamples, vector<float>& infoSamples, double sampleRate, vector<float>& outSamples) {
  const int maxBlocks = outSamples.size()/fftSize;
  // Leave out the last partial block for now.

  auto carrierStart = carrierSamples.begin();
  auto infoStart = infoSamples.begin();
  auto outStart = outSamples.begin();

  IIRFilter carrierHighPassFilter;
  IIRFilter infoHighPassFilter;
  double freq = 5;
  carrierHighPassFilter.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, freq));
  infoHighPassFilter.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, freq));

  //for (int i = 0; i < outSamples.size(); ++i) {
    //outSamples[i] = carrierSamples[i];
  //}
  //return;

  for (int blockIndex = 0; blockIndex < maxBlocks; ++blockIndex) {
    cout << "vocoding block  " << blockIndex << endl;

    auto carrierNext = carrierStart + blockSize;
    auto infoNext = infoStart + blockSize;
    auto outNext = outStart + blockSize;
    vector<float> carrierBlockSamples(carrierStart, carrierNext);
    vector<float> infoBlockSamples(infoStart, infoNext);
    vector<float> outBlockSamples(outStart, outNext);

    vocodeBlock(
      carrierBlockSamples,
      infoBlockSamples,
      carrierHighPassFilter,
      infoHighPassFilter,
      outBlockSamples,
      blockIndex
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

static void vocodeBlock(vector<float>& carrierBlockSamples, vector<float>& infoBlockSamples, IIRFilter& carrierHighPassFilter, IIRFilter& infoHighPassFilter, vector<float>& outBlockSamples, int blockIndex) {

  //auto carrierSample = carrierBlockSamples.begin();
  //auto outSample = outBlockSamples.begin();
  //for (int i = 0; i < outBlockSamples.size(); ++i) {
    //outBlockSamples[i] = carrierSample[i];
  //}
  //return;
  carrierHighPassFilter.processSamples(carrierBlockSamples.data(), carrierBlockSamples.size());
  infoHighPassFilter.processSamples(infoBlockSamples.data(), infoBlockSamples.size());

  // Run a real-only FFT on both signals.
  ComplexFFTArray carrierFFTData;
  ComplexFFTArray infoFFTData;

  for (int i = 0; i < carrierBlockSamples.size(); ++i) {
    carrierFFTData[i] = carrierBlockSamples[i];
    infoFFTData[i] = infoBlockSamples[i];
  }

  if (blockIndex == blockIndexToLog) {
    logSignal("005-info-raw.txt", carrierBlockSamples.size(), carrierBlockSamples.data());
    logSignal("010-carrier-raw.txt", infoBlockSamples.size(), infoBlockSamples.data());
  }

  applyHannWindow(carrierFFTData);
  applyHannWindow(infoFFTData);

  // TODO: Include channel in filename.
  if (blockIndex == blockIndexToLog) {
    logSignal("007-infoHann.txt", fftSize, infoFFTData.data());
    logSignal("030-carrierHann.txt", fftSize, carrierFFTData.data());
  }

  getFFT(carrierFFTData);
  getFFT(infoFFTData);

  if (blockIndex == blockIndexToLog) {
    logSignal("040-carrierFFT.txt", fftSize, carrierFFTData.data());
  }

  FFTArray infoRealBins;
  getReal(infoFFTData, infoRealBins);
  if (blockIndex == blockIndexToLog) {
    logSignal("042-info-fft-real.txt", fftSize, infoRealBins.data());
  }


  FFTArray infoImagBins;
  getImaginary(infoFFTData, infoImagBins);
  if (blockIndex == blockIndexToLog) {
    logSignal("043-info-fft-imag.txt", fftSize, infoImagBins.data());
  }

  // Multiply the reduced real components of the carrier fft by the reduced
  // combined amps.
  FFTArray carrierRealBins;
  getReal(carrierFFTData, carrierRealBins);
  if (blockIndex == blockIndexToLog) {
    logSignal("045-carrier-fft-real.txt", fftSize, carrierRealBins.data());
  }

  FFTArray carrierImagBins;
  getImaginary(carrierFFTData, carrierImagBins);
  if (blockIndex == blockIndexToLog) {
    logSignal("047-carrier-fft-imag.txt", fftSize, carrierImagBins.data());
  }

  // NOTE: we are altering carrierFFTData.
  squareSignal(carrierFFTData.data(), fftSize * 2);
  squareSignal(infoFFTData.data(), fftSize * 2);
  FFTArray carrierFFTSqAdded;
  FFTArray infoFFTSqAdded;
  addRealAndImag(carrierFFTData, carrierFFTSqAdded);
  addRealAndImag(infoFFTData, infoFFTSqAdded);
  if (blockIndex == blockIndexToLog) {
    logSignal("050-carrier-rfft-added.txt", fftSize, carrierFFTSqAdded.data());
    logSignal("055-info-rfft-added.txt", fftSize, infoFFTSqAdded.data());
  }

  FFTArray carrierFFTSqAddedRSqrt;
  FFTArray infoFFTSqAddedSqrt;
  rSqrtSignal(carrierFFTSqAdded.data(), fftSize, carrierFFTSqAddedRSqrt.data());
  sqrtSignal(infoFFTSqAdded.data(), fftSize, infoFFTSqAddedSqrt.data());
  if (blockIndex == blockIndexToLog) {
    logSignal("060-carrierRSqrt.txt", fftSize, carrierFFTSqAddedRSqrt.data());
    logSignal("070-infoSqrt.txt", fftSize, infoFFTSqAddedSqrt.data());
  }

  FFTArray combinedAmpFactors;
  //FloatVectorOperations::multiply(
    //combinedAmpFactors.data(), // dest
    //carrierFFTSqAddedRSqrt.data(),
    //infoFFTSqAddedSqrt.data(),
    //fftSize);
  for (int i = 0; i < combinedAmpFactors.size(); ++i) {
    combinedAmpFactors[i] = carrierFFTSqAddedRSqrt[i] * infoFFTSqAddedSqrt[i];
  }
  printRange("combinedAmpFactors", 5, 15, combinedAmpFactors.data());
  if (blockIndex == blockIndexToLog) {
    logSignal("080-amp-factor-roots-multiplied.txt", fftSize, combinedAmpFactors.data());
  }

  // Turn down the combined amps.
  FFTArray reducedAmpFactors;
  FloatVectorOperations::multiply(
    reducedAmpFactors.data(), combinedAmpFactors.data(), smallifyFactor, fftSize);
  printRange("reducedAmpFactors after reduction", 5, 15, reducedAmpFactors.data());

  FFTArray carrierRealWithReducedAmpFactors;
  FloatVectorOperations::multiply(
    carrierRealWithReducedAmpFactors.data(),
    carrierRealBins.data(),
    reducedAmpFactors.data(),
    fftSize);
  if (blockIndex == blockIndexToLog) {
    logSignal("100-carrier-fft-real-x-reduced-amp-factors.txt", fftSize, carrierRealWithReducedAmpFactors.data());
  }

  // Multiply the imaginary components of the carrier fft by the reduced
  // combined amps.
  FFTArray carrierImagWithReducedAmpFactors;
  FloatVectorOperations::multiply(
    carrierImagWithReducedAmpFactors.data(),
    carrierImagBins.data(),
    reducedAmpFactors.data(),
    fftSize);
  if (blockIndex == blockIndexToLog) {
    logSignal("150-carrier-fft-imag-x-reduced-amp-factors.txt", fftSize, carrierImagWithReducedAmpFactors.data());
  }

  ComplexFFTArray ifftData;
  getIFFT(carrierRealWithReducedAmpFactors, carrierImagWithReducedAmpFactors, ifftData);
  //getIFFT(carrierRealBins, carrierImagBins, ifftData);

  applyHannWindow(ifftData);

  // Copy the results to the channel.
  for (int i = 0; i < fftSize; ++i) {
    outBlockSamples[i] = ifftData[i];
  }
}
