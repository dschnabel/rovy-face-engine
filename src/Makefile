CC= g++
INCLUDES= -I/opt/display-driver -I/opt/audio-driver
CFLAGS= -O2 -Wno-psabi
LIBS= -ldisplay -lpthread -laudio -lpthread -lrovy_ros_helper

all: engine

engine:  engine.o Expression.o
	$(CC) $(CFLAGS) -o engine engine.o Expression.o $(LIBS)

engine.o:  engine.cpp Expression.hpp 
	$(CC) $(CFLAGS) -c engine.cpp $(INCLUDES)

Expression.o:  Expression.cpp Expression.hpp 
	$(CC) $(CFLAGS) -c Expression.cpp $(INCLUDES)

clean:
	rm -f engine *.o
