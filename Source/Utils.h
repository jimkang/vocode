#include <JuceHeader.h>
#include <math.h>
#include "Consts.h"

#pragma once

using namespace juce;
using namespace std;

typedef array<float, fftSize * 2> ComplexFFTArray;
typedef array<float, fftSize> FFTArray;

static void getFFT(const float *samplePtr, int sampleCount, ComplexFFTArray& fftData);
static void getIFFT(FFTArray& realBins, FFTArray& imagBins, ComplexFFTArray& ifftData);
static void getMagnitudes(ComplexFFTArray& fftData, FFTArray& binMagnitudes);
static void printSamples(const char *arrayName, float *array, int arraySize);
static void printRange(const char *arrayName, int lowerBound, int upperBound, float *array);
static void getReal(ComplexFFTArray& fftData, FFTArray& realVals);
static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals);
static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData);

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