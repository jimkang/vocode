#include <JuceHeader.h>
#include <math.h>

using namespace juce;
using namespace std;

const int fftPowerOf2 = 16;
const int fftSize = 1 << fftPowerOf2;
typedef array<float, fftSize * 2> ComplexFFTArray;
typedef array<float, fftSize> FFTArray;
// These come from Miller Puckette's timbre-stamp Pure Data patch.
// I imagine they were determined via experimentation.
const float maxCarrierRSqrt = 9.0;
const float smallifyFactor = 0.00065;

static void vocodeChannel(const float *carrierPtr, const float *infoPtr, const int outLen, float *outPtr);
static void getFFT(const float *samplePtr, int sampleCount, ComplexFFTArray& fftData);
static void getIFFT(FFTArray& realBins, FFTArray& imagBins, ComplexFFTArray& ifftData);
static void getMagnitudes(ComplexFFTArray& fftData, FFTArray& binMagnitudes);
static void printSamples(const char *arrayName, float *array, int arraySize);
static void printRange(const char *arrayName, int lowerBound, int upperBound, float *array);
static void getReal(ComplexFFTArray& fftData, FFTArray& realVals);
static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals);
static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData);

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

// fftData will have real and imaginary parts interleaved.
static void getFFT(const float *samplePtr, int sampleCount, ComplexFFTArray& fftData) {
  fill(fftData.begin(), fftData.end(), 0.0f);
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

  // Run the carrier's reduced real FFT bins and
  // the carrier imaginary bins combined with the
  // magnitude stuff through the inverse FFT to resynthesize.
  printSamples("fftData, after FFT", fftData.data(), sampleLimit);
}

static void getIFFT(FFTArray& realBins, FFTArray& imagBins, ComplexFFTArray& ifftData) {
  zipTogetherComplexArray(realBins, imagBins, ifftData);
  dsp::FFT fft(fftPowerOf2);
  fft.performRealOnlyInverseTransform(ifftData.data());
  printSamples("ifftData", ifftData.data(), fftSize);

  dsp::WindowingFunction<float> window(fftSize, dsp::WindowingFunction<float>::hann);
  window.multiplyWithWindowingTable(ifftData.data(), fftSize);

  printSamples("ifftData, after windowing", ifftData.data(), fftSize);
}

// Assumes fftData will have real and imaginary parts interleaved.
static void getMagnitudes(ComplexFFTArray& fftData, FFTArray& binMagnitudes) {
  for (int i = 0; i < fftData.size(); i += 2) {
    const float realSquared = pow(fftData[i], 2);
    const float imagSquared = pow(fftData[i + 1], 2);
    binMagnitudes[i/2] = sqrt(realSquared + imagSquared);
    if (i >= 10 && i < 21) {
      cout << fftData[i] << " realSquared: " << realSquared << fftData[i + 1] << " imagSquared: " << imagSquared << ", magnitude: " << binMagnitudes[i/2] << endl;
    }
  }
}

// Evens are real, odds are imaginary, I hope.
static void getReal(ComplexFFTArray& fftData, FFTArray& realVals) {
  for (int i = 0; i < fftSize; ++i) {
    realVals[i] = fftData[i * 2];
  }
}

static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals) {
  for (int i = 0; i < fftSize; ++i) {
    imagVals[i] = fftData[i * 2 + 1];
  }
}

static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData) {
  for (int i = 0; i < fftSize; ++i) {
    fftData[i] = realVals[i * 2];
    fftData[i] = imagVals[i * 2 + 1];
  }
}

static void printSamples(const char *arrayName, float *array, int arraySize) {
  cout << arrayName << " example values early: " << array[arraySize/2] << ", " << array[arraySize/2+1] << endl;
  //cout << arrayName << " sample late: " << array[arraySize + arraySize/2] << endl;
  cout << arrayName << " example values late: " << array[arraySize + arraySize/2 + 2] << ", " << array[arraySize + arraySize/2 + 2] << endl;
}

static void printRange(const char *arrayName, int lowerBound, int upperBound, float *array) {
  cout << arrayName << " values ";
  for (int i = lowerBound; i < upperBound; ++i) {
    cout << "Value " << i << ": " << array[i] << ", ";
  }
  cout << endl;
}
