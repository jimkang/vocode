#include <JuceHeader.h>
#include <math.h>
#include "Consts.h"

#pragma once

using namespace juce;
using namespace std;

static bool verbose = false;

typedef array<float, fftSize * 2> ComplexFFTArray;
typedef array<float, fftSize> FFTArray;

static void getIFFT(FFTArray& realBins, FFTArray& imagBins, ComplexFFTArray& ifftData);
static void getMagnitudes(ComplexFFTArray& fftData, FFTArray& binMagnitudes, bool addTinyNumber);
static void printSamples(const char *arrayName, float *array, int arraySize);
static void printRange(const char *arrayName, int lowerBound, int upperBound, const float *array);
static void getReal(ComplexFFTArray& fftData, FFTArray& realVals);
static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals);
static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData);
static float reciprocalSqRt(float bin);

static void applyHannWindow(float *signalBlock, int size) {
  dsp::WindowingFunction<float> window(size, dsp::WindowingFunction<float>::hann, false);
  window.multiplyWithWindowingTable(signalBlock, size);

  //float maxOrig = 0.0;
  //float maxNew = 0.0;
//
  //// TODO: Hann curve should be memoized.
  //for (int i = 0; i < size; ++i) {
    //const float orig = signalBlock[i];
    //const float hann = (cos( (i*1.0/size * 2.0 + 1.0)*M_PI ) + 1)/2.0;
    //signalBlock[i] = orig * hann;
    //if (orig > maxOrig) {
      //maxOrig = orig;
    //}
    //if (signalBlock[i] > maxNew) {
      //maxNew = signalBlock[i];
    //}
  //}
//
  //cout << "maxOrig: " << maxOrig << ", maxNew: " << maxNew << endl;
}

// fftData will have real and imaginary parts interleaved.
static void getFFT(ComplexFFTArray& fftData) {
  dsp::FFT fft(fftPowerOf2);
  fft.performRealOnlyForwardTransform(fftData.data(), true);
  // The second param, dontCalculateNegativeFrequencies, is ignored in the fallback
  // FFT impl. so we have to zero the second half ourselves if we don't want it.
  //const int halfByteLength = fftData.size() * sizeof(float) / 2;
  //zeromem(fftData.data() + halfByteLength, halfByteLength);
  const int halfLength = fftData.size() / 2;
  for (int i = 0; i < halfLength; ++i) {
    fftData[halfLength + i] = 0;
  }
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

    //if (verbose && i >= 10 && i < 21) {
      //cout << fftData[i] << " realSquared: " << realSquared << fftData[i + 1] << " imagSquared: " << imagSquared << ", magnitude: " << binMagnitudes[i/2] << endl;
    //}
  }
}

// Evens are real, odds are imaginary, I hope.
static void getReal(ComplexFFTArray& fftData, FFTArray& realVals) {
  //for (int i = 0; i < fftSize; ++i) {
    //realVals[i] = fftData[i * 2];
  //}
  dsp::Complex<float> *complexFFTData = reinterpret_cast<dsp::Complex<float> *>(fftData.data());
  for (int i = 0; i < fftSize; ++i) {
    dsp::Complex<float> comp = complexFFTData[i];
    realVals[i] = comp.real();
  }
}

static void getImaginary(ComplexFFTArray& fftData, FFTArray& imagVals) {
  //for (int i = 0; i < fftSize; ++i) {
    //imagVals[i] = fftData[i * 2 + 1];
  //}
  dsp::Complex<float> *complexFFTData = reinterpret_cast<dsp::Complex<float> *>(fftData.data());
  for (int i = 0; i < fftSize; ++i) {
    imagVals[i] = complexFFTData[i].imag();
  }
}

static void zipTogetherComplexArray(FFTArray& realVals, FFTArray& imagVals, ComplexFFTArray& fftData) {
  //for (int i = 0; i < fftSize; ++i) {
    //fftData[i * 2] = realVals[i];
    //fftData[i * 2 + 1] = imagVals[i];
  //}
  dsp::Complex<float> *complexFFTData = reinterpret_cast<dsp::Complex<float> *>(fftData.data());
  for (int i = 0; i < fftSize; ++i) {
    complexFFTData[i] = complex<float>(realVals[i], imagVals[i]);
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
    float val = array[i];
    if (val == 0) {
      val = tinyNumber;
    }
    outArray[i] = reciprocalSqRt(val);
  }
}

static void sqrtSignal(const float *array, int size, float *outArray) {
  for (int i = 0; i < size; ++i) {
    // TODO: Move this out of here and just add
    // it to the incoming signal ahead of time.
    float val = array[i];
    if (val == 0) {
      val = tinyNumber;
    }
    outArray[i] = sqrt(val);
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
    sumArray[i] = compFFTArray[i] + compFFTArray[i + fftSize];
  }
}
