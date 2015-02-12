CXXFLAGS = -g -O0 -Wall -Wextra -lpthread -I..

all:	ThreadTest

ThreadTest:	ThreadTest.o ../ThreadPool.o ../ThreadPool.h
	g++ $(CXXFLAGS) ThreadTest.cxx ../ThreadPool.cxx -o ThreadTest

clean:
	rm ThreadTest *.o

%.o:	%.cxx Makefile
	g++ -c $(CXXFLAGS) -o $@ $<

