#pragma once

#include <math.h>

const int fftPowerOf2 = 10;
const int fftSize = 1 << fftPowerOf2;

// These come from Miller Puckette's timbre-stamp Pure Data patch.
// I imagine they were determined via experimentation.
const float maxCarrierMag = 9;
const float smallifyFactor = 0.00065;
const float tinyNumber = pow(10, -20);

const int overlapSize = pow(2, fftPowerOf2 - 1);

// TODO: Make this an arg.
const char *baseLogPath = "../../../logs/";
