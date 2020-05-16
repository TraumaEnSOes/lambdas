CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic -O0 -g -ggdb
LDFLAGS=
LOADLIBES=
LDLIBS=
CPPFLAGS=
CXX=clang++
LD=clang++

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $^

.PHONY: all
all: test

test: test.cpp
	$(LD) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

.PHONY: clean
clean:
	rm -f *.o
	rm -f test1

