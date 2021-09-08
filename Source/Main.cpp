/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>

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

  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  juce::AudioFormatReader *carrierReader = formatManager.createReaderFor(carrierFile);
  if (carrierReader == NULL) {
    std::cerr << "Could not read carrier file." << std::endl;
    return 1;
  }
  juce::AudioFormatReader *infoReader = formatManager.createReaderFor(infoFile);
  if (infoReader == NULL) {
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

  // Just make some kind of nonsense output out of it for now.
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

  for (int ch = 0; ch < channelCount; ++ch) {
    const float *carrierPtr = carrierBuffer.getReadPointer(ch);
    const float *infoPtr = infoBuffer.getReadPointer(ch);
    float *outPtr = outBuffer.getWritePointer(ch);

    for (int i = 0; i < outLen; ++i) {
      outPtr[i] = i % 2 == 0 ? carrierPtr[i] : infoPtr[i];
    }
  }

  juce::FileOutputStream outStream(outFile, outLen * channelCount);
  if (outStream.failedToOpen()) {
    std::cerr << "Could not open output file." << std::endl;
    return 1;
  }

  juce::WavAudioFormat wavFormat;
  std::cout << "channelCount: " << channelCount << ", outLen: " << outLen << std::endl;
  juce::AudioFormatWriter *writer = wavFormat.createWriterFor(
    &outStream, sampleRate, channelCount, bitsPerSample, NULL, 0
  );
  if (writer == NULL) {
    std::cerr << "Could not write to output file." << std::endl;
    return 1;
  }
  writer->writeFromAudioSampleBuffer(outBuffer, 0, outLen);

  delete writer;

  return 0;
}
