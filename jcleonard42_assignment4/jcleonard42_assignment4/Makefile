main: server.cpp
	g++ -c -o server.o server.cpp
	g++ -Wall -pedantic server.cpp ./libastyle-2.06d.so -o server
run:
	LD_LIBRARY_PATH=$$(find . -name "server.cpp" -print -quit | xargs dirname): ./server
clean:
	rm -f server.o server
