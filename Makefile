# This isn't the Makefile for building the project. That's in Builds/LinuxMakefile. This is just utils.

try:
	cd Builds/LinuxMakefile/build && ./vocode ../../../example-media/donut.wav ../../../example-media/talking.wav ../../../example-media/result.wav
