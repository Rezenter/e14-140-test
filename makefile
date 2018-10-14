CC = g++
CFLAGS = -std=c++11 -D_REENTERANT -DLCOMP_LINUX=1 -rdynamic -O2
LIBS = -I/usr/include/jsoncpp/ -I/usr/include/labbot/ -L$(CURDIR)/include -llcomp -ljsoncpp -ldl -lpthread

all: tester.exe
	rm tester.o #debug

tester.exe: tester.o ExampleModule.o
	 $(CC) -o tester.exe tester.o ExampleModule.o $(CFLAGS) $(LIBS) 

tester.o: tester.cpp
	 $(CC) $(LIBS) $(CFLAGS) -c tester.cpp

ExampleModule.o: ExampleModule.cpp
	$(CC) $(LIBS) $(CFLAGS) -c ExampleModule.cpp

clean:
	 rm tester.o tester.exe

run: tester.exe
	./tester.exe
