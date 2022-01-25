#pragma once

#include <math.h>

const int fftPowerOf2 = 10;
const int fftSize = 1 << fftPowerOf2;
const int blockSize = fftSize;
const int blockSizeInBytes = blockSize * sizeof(float);
// These come from Miller Puckette's timbre-stamp Pure Data patch.
// I imagine they were determined via experimentation.
const float maxCarrierMag = 9;
const float smallifyFactor = 1.0;//0.00065;
const float tinyNumber = pow(10, -20);

const int overlapFactor = 4;//pow(2, fftPowerOf2 - 1);
const int overlapOffset = blockSize/overlapFactor;

const int closeEnoughToZero = pow(10, -6);

// TODO: Make this an arg.
const char *baseLogPath = "../../../logs/";
const int blockIndexToLog = 68;
