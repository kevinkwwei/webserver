main: main.o httpclient.o
		g++ -o main.exe main.o httpclient.o
main.o: main.cpp
		g++ -c -g main.cpp
httpclient.o: httpclient.cpp
		g++ -c -g httpclient.cpp
clean:
		-rm -rf *.o