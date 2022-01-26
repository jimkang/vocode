#include <stdio.h>
#include <iostream>
#include <fstream>
#include "../Source/Sqrt.h"
#include <vector>

using namespace std;

const int maxLineLength = 100;

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: try-rsqrt <path to text file with input values, one per line> > results.txt\n");
    return 1;
  }

  char *textPath = argv[1];
  FILE *file = fopen(textPath, "rb");

  if (!file) {
    cerr << "Could not open file: " << textPath << endl;
    return 1;
  }

  cout.precision(14);

  char line[maxLineLength] = {0};
  vector<float> inputFloats;

  while (fgets(line, maxLineLength, file)) {
    inputFloats.push_back(stof(line));
  }

  vector<float> outputFloats(inputFloats.size());
  rSqrtSignal(inputFloats.data(), inputFloats.size(), outputFloats.data());

  for (auto it = outputFloats.begin(); it != outputFloats.end(); ++it) {
    cout << *it << endl;
  }

  if (fclose(file)) {
    cout << "Could not close file: " << textPath << endl;
    return 1;
  }

  return 0;
}
