/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Vocode.h"
#include "Reconstruct.h"
#include "WriteAudio.h"

//==============================================================================
int main (int argc, char* argv[])
{
  if (argc < 4) {
    std::cerr << "Usage: vocode carrier.wav info.wav result.wav" << std::endl;
    return 1;
  }

  juce::File carrierFile(argv[1]);
  if (!carrierFile.existsAsFile()) {
    std::cerr << "Carrier file does not exist." << std::endl;
    return 1;
  }
  juce::File infoFile(argv[2]);
  if (!infoFile.existsAsFile()) {
    std::cerr << "Info file does not exist." << std::endl;
    return 1;
  }
  const char *outFilePath = argv[3];

  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  juce::AudioFormatReader *carrierReader = formatManager.createReaderFor(carrierFile);
  if (carrierReader == nullptr) {
    std::cerr << "Could not read carrier file." << std::endl;
    return 1;
  }
  juce::AudioFormatReader *infoReader = formatManager.createReaderFor(infoFile);
  if (infoReader == nullptr) {
    std::cerr << "Could not read info file." << std::endl;
    return 1;
  }

  juce::AudioBuffer<float> carrierBuffer;
  carrierBuffer.setSize(carrierReader->numChannels, carrierReader->lengthInSamples);
  carrierReader->read(&carrierBuffer, 0, carrierReader->lengthInSamples, 0, true, true);
  std::cout << "Read " << carrierReader->lengthInSamples << " samples from carrier." << std::endl;

  const double sampleRate = carrierReader->sampleRate;
  const int bitsPerSample = carrierReader->bitsPerSample;

  juce::AudioBuffer<float> infoBuffer;
  infoBuffer.setSize(infoReader->numChannels, infoReader->lengthInSamples);
  infoReader->read(&infoBuffer, 0, infoReader->lengthInSamples, 0, true, true);
  std::cout << "Read " << infoReader->lengthInSamples << " samples from info." << std::endl;

  delete infoReader;
  delete carrierReader;

  int outLen = carrierReader->lengthInSamples;
  if (infoReader->lengthInSamples < outLen) {
    outLen = infoReader->lengthInSamples;
  }
  int channelCount = carrierReader->numChannels;
  if (infoReader->numChannels < channelCount) {
    channelCount = infoReader->numChannels;
  }

  juce::AudioBuffer<float> outBuffer;
  outBuffer.setSize(channelCount, outLen);

  DebugSignalsForChannels debugSignalsForChannels;

  for (int ch = 0; ch < channelCount; ++ch) {
    DebugSignals *debugSignals = new DebugSignals({
      {"carrierHann", new float[outLen]},
      {"infoHann", new float[outLen]},
      {"carrierFFT", new float[outLen]},
      {"infoFFT", new float[outLen]},
      {"carrierFFTSqAdded", new float[outLen]},
      {"infoFFTSqAdded", new float[outLen]},
      {"carrierFFTSqAddedRSqrt", new float[outLen]},
      {"infoFFTSqAddedSqrt", new float[outLen]}
    });
    debugSignalsForChannels[ch] = debugSignals;
  };

  DebugSignals *debugSignals = debugSignalsForChannels[0];
  float *signal = (*debugSignals)["carrierHann"];

  //reconstruct(carrierBuffer, outBuffer);
  vocode(carrierBuffer, infoBuffer, debugSignalsForChannels, outBuffer);

  writeBufferToFile(outBuffer, sampleRate, bitsPerSample, outFilePath);

  DebugSignals *debugSignalsLeft = debugSignalsForChannels[0];

  DebugSignals *debugSignalsRight = NULL;
  if (channelCount > 1) {
    debugSignalsRight = debugSignalsForChannels[1];
  }

  for (auto it = debugSignalsLeft->begin(); it != debugSignals->end(); ++it) {
    float *signalLeft = it->second;
    float *signalRight = NULL;
    // Assumption: Signal names are the same in the DebugSignals for each channel.
    if (debugSignalsRight) {
      signalRight = (*debugSignalsRight)[it->first];
    }

    float *signalsByChannel[1] = { signalLeft };
    float *dualSignalsByChannel[2] = { signalLeft, signalRight };

    string filePath = string("../../../debug-snapshots/juce/").append(it->first);

    if (!writeArrayToFile(signalRight ? dualSignalsByChannel : signalsByChannel, channelCount, outLen,
      sampleRate, bitsPerSample, filePath.c_str())) {

      cerr << "Unable to write debug signal for " << it->first << endl;
    }
  }

  for (int ch = 0; ch < channelCount; ++ch) {
    DebugSignals *debugSignals = debugSignalsForChannels[ch];
    for (auto it = debugSignals->begin(); it != debugSignals->end(); ++it) {
      delete it->second;
    }
    delete debugSignals;
  };
  return 0;
}
