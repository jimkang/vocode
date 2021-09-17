#pragma once

const int fftPowerOf2 = 12;
const int fftSize = 1 << fftPowerOf2;

// These come from Miller Puckette's timbre-stamp Pure Data patch.
// I imagine they were determined via experimentation.
const float maxCarrierMag = 9.0;
const float smallifyFactor = 0.065;

const float overlapAmount = 2.0;
