# This isn't the Makefile for building the project. That's in Builds/LinuxMakefile. This is just utils.

try: clean-logs
	cd Builds/LinuxMakefile/build && ./vocode ../../../example-media/donut.wav ../../../example-media/talking.wav ../../../example-media/result.wav

debug: clean-logs
	cd Builds/LinuxMakefile/build && gdb --args vocode ../../../example-media/donut.wav ../../../example-media/talking.wav ../../../example-media/result.wav

clean-logs:
	rm logs/*-b.txt || echo "Logs clean already."

logs-to-sheet:
	node tools/logs-to-csv.js logs > logs/logs.csv

show-pd-snaps:
	node tools/wrap-images-in-html.js debug-snapshots/pd tools/page-template.html tools/entry-template.html > debug-snapshots/pd/index.html
	firefox debug-snapshots/pd/index.html

rsqrt: tools/try-rsqrt
	./tools/try-rsqrt logs/050-carrier-rfft-added-a.txt > logs/060-carrier-rsqrt-experimental.txt

tools/try-rsqrt:
	g++ tools/try-rsqrt.c -o tools/try-rsqrt -std=c++11

build-try-rsqrt:
	g++ tools/try-rsqrt.c -o tools/try-rsqrt -std=c++11
