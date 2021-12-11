# This isn't the Makefile for building the project. That's in Builds/LinuxMakefile. This is just utils.

try: clean-logs
	cd Builds/LinuxMakefile/build && ./vocode ../../../example-media/donut.wav ../../../example-media/talking.wav ../../../example-media/result.wav

debug: clean-logs
	cd Builds/LinuxMakefile/build && gdb --args vocode ../../../example-media/donut.wav ../../../example-media/talking.wav ../../../example-media/result.wav

clean-logs:
	rm juce-logs/* || echo "Logs clean already."

show-pd-snaps:
	node tools/wrap-images-in-html.js debug-snapshots/pd tools/page-template.html tools/entry-template.html > debug-snapshots/pd/index.html
	firefox debug-snapshots/pd/index.html
