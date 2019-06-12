all: mirror_client

mirror_client: mirror_client.o List.o
	g++ -o mirror_client mirror_client.o List.o

mirror_client.o: mirror_client.cpp
	g++ -c mirror_client.cpp

List.o: List.cpp
	g++ -c List.cpp

clean:	
	rm mirror_client *.o
