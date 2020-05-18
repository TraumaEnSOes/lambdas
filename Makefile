CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic -O0 -g -ggdb
LDFLAGS=
LOADLIBES=
LDLIBS=
CPPFLAGS=
CXX=g++
LD=g++

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $^

.PHONY: all
all: test

test: test.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f *.o
	rm -f test

