CXXFLAGS:= -Wall -Wno-unused -g -I. ${CXXFLAGS}

TARGETS:= ecl-bench ecl-test

all: ${TARGETS}

.PHONY: clean
clean:
	rm -f ${TARGETS}

ecl-bench ecl-test: $(wildcard ecl/*.hpp)

ecl-bench: bench/ecl-bench.cpp

ecl-test: test/ecl-test.cpp

${TARGETS}:
	${CXX} ${CXXFLAGS} -o $@ $(filter %.cpp,$^)
