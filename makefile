hlslp: hlslp.hpp hlslp.cpp makefile parsegen
	g++ -std=c++17 -lparsegen -L/Users/bennywwg/Build/parsegen-cpp/src -O3 hlslp.cpp -o hlslp

parsegen: ./parsegen/src/libparsegen.a
	./configure-linux.sh

test: hlslp
	./hlslp ./shaders.hlsl

clean:
	rm -f hlslp