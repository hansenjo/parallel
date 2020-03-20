CXXFLAGS = -std=c++17 -g -O -Wall -Wextra  -I.. -pthread

all:	ThreadTest

ThreadTest:	ThreadTest.o ../ThreadPool.hpp
	g++ $(CXXFLAGS) ThreadTest.cxx -o ThreadTest

clean:
	rm ThreadTest *.o

%.o:	%.cxx Makefile
	g++ -c $(CXXFLAGS) -o $@ $<

