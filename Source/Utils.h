#include <JuceHeader.h>
#include <math.h>
#include "Consts.h"

#pragma once

using namespace juce;
using namespace std;

static bool verbose = true;

typedef array<float, fftSize * 2> ComplexFFTArray;
typedef array<float, fftSize> FFTArray;
typedef map<string, float *> DebugSignals;
typedef array<DebugSignals *, 2> DebugSignalsForChannels;

static void getIFFT(FFTArray& realBins, FFTArray& imagBins, ComplexFFTArray& ifftData);
static void getMagnitudes(ComplexFFTArray& fftData, FFTArray& binMagnitudes, bool addTinyNumber);
static void printSamples(const char *arrayName, float *array, int arraySize);
static void printRange(const char *arrayName, int lowerBound, int upperBound, const float *array);
static void getReal(ComplexFFTArray& fftData, FFTArray& realVals);
static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals);
static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData);
static float reciprocalSqRt(float bin);

static void applyHannWindow(ComplexFFTArray& fftData) {
  dsp::WindowingFunction<float> window(fftSize, dsp::WindowingFunction<float>::hann);
  window.multiplyWithWindowingTable(fftData.data(), fftSize);
}

// fftData will have real and imaginary parts interleaved.
static void getFFT(ComplexFFTArray& fftData) {
  dsp::FFT fft(fftPowerOf2);
  fft.performRealOnlyForwardTransform(fftData.data(), true);
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
static void getMagnitudes(ComplexFFTArray& fftData, FFTArray& binMagnitudes, bool addTinyNumber) {
  for (int i = 0; i < fftSize * 2; i += 2) {
    const float real = fftData[i];
    const float imag = fftData[i + 1];
    const float realSquared = real * real;
    const float imagSquared = imag * imag;
    float sum = realSquared + imagSquared;
    // I don't know that adding this tiny number
    // matters.
    if (addTinyNumber) {
      sum += tinyNumber;
    }
    binMagnitudes[i/2] = reciprocalSqRt(sum);

    if (verbose && i >= 10 && i < 21) {
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
    fftData[i * 2] = realVals[i];
    fftData[i * 2 + 1] = imagVals[i];
  }
}

static void printSamples(const char *arrayName, float *array, int arraySize) {
  if (!verbose) {
    return;
  }
  cout << arrayName << " example values early: " << array[arraySize/2] << ", " << array[arraySize/2+1] << endl;
  //cout << arrayName << " sample late: " << array[arraySize + arraySize/2] << endl;
  cout << arrayName << " example values late: " << array[arraySize + arraySize/2 + 2] << ", " << array[arraySize + arraySize/2 + 2] << endl;
}

static void printRange(const char *arrayName, int lowerBound, int upperBound, const float *array) {
  if (!verbose) {
    return;
  }
  cout << arrayName << " values ";
  for (int i = lowerBound; i < upperBound; ++i) {
    cout << i << ": " << array[i] << "| ";
  }
  cout << endl;
}

static float reciprocalSqRt(float bin) {
  return 1.0 / sqrt(bin);
}

static void saveArrayToDebug(
  const float *array,
  int writeStart,
  int length,
  const string& debugName,
  DebugSignals& debugSignals) {

  float *writePtr = debugSignals[debugName];
  for (int i = 0; i < length; ++i) {
    writePtr[writeStart + i] += array[i];
  }
}
