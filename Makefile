rewrite_linux: rewrite_linux.cpp
	g++ -Wall -Wextra rewrite_linux.cpp -o build/rewrite_linux -lstdc++ -lm

clean:
	rm -f build/rewrite_linux

run: rewrite_linux
	xterm -fa 'xft:ProggyTiny:style=Bold:size=8'  -geometry 80x42  -e "./build/rewrite_linux; exec bash"
