CXX := c++
CXXFLAGS := -std=c++23 -Wall -Wextra -pedantic
LDLIBS := -lmncl

.PHONY: all run clean vsm

all: lifegame.bin lifegame

lifegame.bin: lifegame.c
	mnclc $< -e lifegame -o $@

lifegame: main.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

run: all
	./lifegame sample.txt

vsm:
	CODEGEN_DUMP_VSM=1 mnclc lifegame.c -e lifegame -o lifegame.bin 2> lifegame.vsm

clean:
	rm -f lifegame lifegame.bin lifegame.vsm
