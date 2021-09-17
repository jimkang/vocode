#pragma once

const int fftPowerOf2 = 16;
const int fftSize = 1 << fftPowerOf2;
// These come from Miller Puckette's timbre-stamp Pure Data patch.
// I imagine they were determined via experimentation.
const float maxCarrierRSqrt = 9.0;
const float smallifyFactor = 0.00065;
