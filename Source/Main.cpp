/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Vocode.h"

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
  juce::File outFile(argv[3]);
  outFile.deleteFile();

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

  vocode(carrierBuffer, infoBuffer, outBuffer);
  // Creating an outStream with the FileOutputStream on the stack creates
  // a problem because the AudioFormatWriter expects to be able to delete
  // it when it is destroyed. So, when we delete the writer below, it
  // will try to delete the outStream, which it cannot!
  // https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_formats/format/juce_AudioFormatWriter.cpp#L61
  //juce::FileOutputStream outStream(outFile, outLen * channelCount);
  std::unique_ptr<juce::FileOutputStream> outStream = outFile.createOutputStream();
  if (outStream->failedToOpen()) {
    std::cerr << "Could not open output file." << std::endl;
    return 1;
  }

  juce::WavAudioFormat wavFormat;
  std::cout << "channelCount: " << channelCount << ", outLen: " << outLen << ", bitsPerSample: " << bitsPerSample << std::endl;
  juce::AudioFormatWriter *writer = wavFormat.createWriterFor(
    outStream.get(), sampleRate, channelCount, bitsPerSample, {}, 0
  );
  if (writer == nullptr) {
    std::cerr << "Could not write to output file." << std::endl;
    return 1;
  }
  outStream.release();
  writer->writeFromAudioSampleBuffer(outBuffer, 0, outLen);

  delete writer;

  return 0;
}
