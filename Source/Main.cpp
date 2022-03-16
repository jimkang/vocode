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
int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: vocode carrier.wav info.wav result.wav OR vocode --step stepname in.txt out.txt"
                  << std::endl;
        return 1;
    }

    if (argv[1] == "--step") {
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

    cout << "Block size: " << blockSize << endl;
    //reconstruct(carrierBuffer, outBuffer);
    vocode(carrierBuffer, infoBuffer, sampleRate, outBuffer);

    writeBufferToFile(outBuffer, sampleRate, bitsPerSample, outFilePath);

    return 0;
}
