hlslp: hlslp.hpp hlslp.cpp makefile
	g++ -std=c++17 -lparsegen -L./parsegen/src -O3 hlslp.cpp -o hlslp

test: hlslp
	./hlslp ./shaders.hlsl

clean:
	rm -f hlslp
