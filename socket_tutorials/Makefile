CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: udp_server udp_client tcp_client tcp_server udp_client_select udp_server_select udp_client_extra udp_server_extra

udp_server: udp_server.o
	    $(CC) -o udp_server udp_server.o  

udp_client: udp_client.o
	    $(CC) -o udp_client udp_client.o

tcp_server: tcp_server.o
	    $(CC) -o tcp_server tcp_server.o  

tcp_client: tcp_client.o
	    $(CC) -o tcp_client tcp_client.o

udp_client_select: udp_client_select.o
	    $(CC) -o udp_client_select udp_client_select.o

udp_server_select: udp_server_select.o
	    $(CC) -o udp_server_select udp_server_select.o

udp_client_extra: udp_client_extra.o
	    $(CC) -o udp_client_extra udp_client_extra.o

udp_server_extra: udp_server_extra.o
	    $(CC) -o udp_server_extra udp_server_extra.o

clean:
	rm *.o
	rm udp_server 
	rm udp_client
	rm tcp_server 
	rm tcp_client
	rm udp_client_select
	rm udp_server_select
	rm udp_client_extra
	rm udp_server_extra

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


