#include <JuceHeader.h>
#include <math.h>
#include "Consts.h"

#pragma once

using namespace juce;
using namespace std;

static bool verbose = true;

typedef array<dsp::Complex<float>, fftSize> ComplexFFTArray;
typedef array<float, fftSize> FFTArray;
typedef map<string, float *> DebugSignals;
typedef array<DebugSignals *, 2> DebugSignalsForChannels;

static void getIFFT(FFTArray& realBins, FFTArray& imagBins, ComplexFFTArray& ifftData);
static void printSamples(const char *arrayName, float *array, int arraySize);
static void printRange(const char *arrayName, int lowerBound, int upperBound, const float *array);
static void getReal(ComplexFFTArray& fftData, FFTArray& realVals);
static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals);
static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData);
static float reciprocalSqRt(float bin);


// fftData will have real and imaginary parts interleaved.
static void getFFT(ComplexFFTArray& fftData) {
  dsp::FFT fft(fftPowerOf2);
  fft.performRealOnlyForwardTransform(reinterpret_cast<float *>(fftData.data()));
}

static void getIFFT(FFTArray& realBins, FFTArray& imagBins, ComplexFFTArray& ifftData) {
  zipTogetherComplexArray(realBins, imagBins, ifftData);
  dsp::FFT fft(fftPowerOf2);
  fft.performRealOnlyInverseTransform(reinterpret_cast<float *>(ifftData.data()));

  FFTArray realResult;
  getReal(ifftData, realResult);
  printSamples("ifftData", realResult.data(), fftSize);

  dsp::WindowingFunction<float> window(fftSize, dsp::WindowingFunction<float>::hann);
  window.multiplyWithWindowingTable(realResult.data(), fftSize);

  printSamples("ifftData, after windowing", realResult.data(), fftSize);
}

static void getReal(ComplexFFTArray& fftData, FFTArray& realVals) {
  transform(fftData.begin(), fftData.end(), realVals.begin(), [](complex<float> c) -> float { return c.real(); });
}

static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals) {
  transform(fftData.begin(), fftData.end(), imagVals.begin(), [](dsp::Complex<float> c) -> float { return c.imag(); });
}

static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData) {
  for (int i = 0; i < fftSize; ++i) {
    fftData[i] = complex<float>(realVals[i], imagVals[i]);
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

static void rSqrtSignal(const float *array, int size, float *outArray) {
  for (int i = 0; i < size; ++i) {
    outArray[i] = reciprocalSqRt(array[i]);
  }
}

static void sqrtSignal(const float *array, int size, float *outArray) {
  for (int i = 0; i < size; ++i) {
    outArray[i] = sqrt(array[i]);
  }
}

// In-place
static void squareSignal(float *array, int size) {
  for (int i = 0; i < size; ++i) {
    array[i] *= array[i];
  }
}

static void addRealAndImag(const ComplexFFTArray& compFFTArray, FFTArray& sumArray) {
  for (int i = 0; i < fftSize; ++i) {
    sumArray[i] = compFFTArray[i].real() + compFFTArray[i].imag();
  }
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
