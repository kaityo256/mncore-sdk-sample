CXX := c++
CXXFLAGS := -std=c++23
LDLIBS := -lmncl

.PHONY: all run clean vsm

all: add.bin add_host

add.bin: add.c
	mnclc $< -e add -o $@

add_host: main.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

run: all
	./add_host

vsm:
	CODEGEN_DUMP_VSM=1 mnclc add.c -e add -o add.bin 2> add.vsm

clean:
	rm -f add.bin add_host add.vsm
