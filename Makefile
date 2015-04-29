all : echo_server_n echo_client_n

echo_server_n : echo_server_n.o CEchoIO.o
	g++ -g -o  echo_server_n echo_server_n.o CEchoIO.o

echo_client_n : echo_client_n.o
	g++ -g -o  echo_client_n echo_client_n.o
	

CEchoIO.o : CEchoIO.h

clean:
	rm echo_server_n echo_client_n echo_server_n.o echo_client_n.o CEchoIO.o
