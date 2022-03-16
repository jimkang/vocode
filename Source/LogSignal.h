#include <iostream>
#include <fstream>
#include "Consts.h"

#pragma once

using namespace std;

// TODO: Clear logs at beginning of program.
void logSignal(const char *fileName, int signalLength, const float *signal) {
    string filePath = baseLogPath;
    filePath += fileName;
    ofstream out(filePath.c_str(), ios_base::app);
    out.precision(14);
    for (int i = 0; i < signalLength; ++i) {
        out << signal[i] << endl;
    }
    out.close();
}
