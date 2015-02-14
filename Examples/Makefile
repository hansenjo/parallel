CXXFLAGS = -g -O -Wall -Wextra -lpthread -I..

all:	ThreadTest

ThreadTest:	ThreadTest.o ../ThreadPool.hpp
	g++ $(CXXFLAGS) ThreadTest.cxx -o ThreadTest

clean:
	rm ThreadTest *.o

%.o:	%.cxx Makefile
	g++ -c $(CXXFLAGS) -o $@ $<

