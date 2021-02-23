CC= g++
INCLUDES= -I/opt/display-driver -I/opt/audio-driver -I/opt/ros/melodic/include
CFLAGS= -O2 -Wno-psabi -L/opt/ros/melodic/lib
LIBS= -ldisplay -lpthread -laudio -lpthread -lroscpp -lrosconsole_backend_interface -lrosconsole_log4cxx -lcpp_common -lxmlrpcpp -lroscpp_serialization -lrosconsole -lrostime

all: engine

engine:  engine.o Expression.o
	$(CC) $(CFLAGS) -o engine engine.o Expression.o $(LIBS)

engine.o:  engine.cpp Expression.hpp 
	$(CC) $(CFLAGS) -c engine.cpp $(INCLUDES)

Expression.o:  Expression.cpp Expression.hpp 
	$(CC) $(CFLAGS) -c Expression.cpp $(INCLUDES)

clean:
	rm -f engine *.o
