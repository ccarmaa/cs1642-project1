/*
 * CS 1652 Project 1 
 * (c) Jack Lange, 2020
 * (c) Amy Babay, 2022
 * (c) Cameron Frencho and Bridget Brinkman
 * 
 * Computer Science Department
 * University of Pittsburgh
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100


static int 
handle_connection(int sock) 
{
 
    char * ok_response_f  = "HTTP/1.0 200 OK\r\n"						\
                            "Content-type: text/plain\r\n"				\
                            "Content-length: %d \r\n\r\n";
    
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"			\
                            "Content-type: text/html\r\n\r\n"			\
                            "<html><body bgColor=black text=white>\n"	\
                            "<h2>404 FILE NOT FOUND</h2>\n"				\
                            "</body></html>\n";
    
  //(void)notok_response;  // DELETE ME
  //(void)ok_response_f;   // DELETE ME
    // ok this is literally the same as server1 :DDDD
    // so im just pasting it in  

    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE); //clear buf
    char filename[FILENAMESIZE];
    char header[BUFSIZE];

    char method[10], path[FILENAMESIZE], version[10];
    FILE *file;
    int len, bytes_read, file_size;

    /* first read loop -- get request and headers*/
    // from tcp_server.c


    /* uh im concerned that we need to loop until we see /n/n/n/n or whatever so im gonna save this incase i fail miserably :D
    if ((len = recv(sock, buf, BUFSIZE - 1, 0)) <= 0) {
        perror("http_server1: recv error");
        return -1;
    }
    buf[len] = '\0'; // null term so we can print it
    */
    int total_len = 0;
    while(total_len < BUFSIZE -1){
        /*
            here me out this means:
            - start writing at buf+total_len (so we dont overwrite)
            - write at most BUFSIZE - 1 - total_len bytes (so we dont overflow)
        */

        if((len = recv(sock, buf + total_len, BUFSIZE - 1 - total_len, 0)) <= 0) {
            perror("http_server1: recv error");
            return -1;
        }
        total_len += len;
        buf[total_len] = '\0';

        // ok if our thing has the /n/n/n/n stuff we have the full thing, so we can break
        if (strstr(buf, "\r\n\r\n") != NULL) {
            break;
        }
    }

    
    
    /* parse request to get file name */
    /* Assumption: this is a GET request and filename contains no spaces*/
    if (sscanf(buf, "%s %s %s", method, path, version) != 3) {
        fprintf(stderr, "http_server1: failed to parse request\n");
        return -1;
    }

    // extract filename from path
    // skip leading '/' if there is one
    if (path[0] == '/') {
        strncpy(filename, path + 1, FILENAMESIZE - 1);
    } else {
        strncpy(filename, path, FILENAMESIZE - 1);
    }
    filename[FILENAMESIZE - 1] = '\0'; // ensure null term

    /* open and read the file */
	if ((file = fopen(filename, "rb")) == NULL) {
    
    // send 404 if not found
        if (send(sock, notok_response, strlen(notok_response), 0) <= 0) {
            perror("http_server1: send error");
            return -1;
        }
        return 0;
    }
	
    // resource: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    // get the file size 
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    

    /* send response */ 
    // send header
    sprintf(header, ok_response_f, file_size); // sprintf(destbuff, format str, size)
    // basically same as tcp  server. but we are sending header rn
    if (send(sock, header, strlen(header), 0) <= 0) {
        perror("http_server1: send error");
        fclose(file);
        return -1;
    }
    // send file contents
    // kinda just keeps reading and sending til there aint more to read
    while ((bytes_read = fread(buf, 1, BUFSIZE, file)) > 0) {
        int sent = 0;
        while (sent < bytes_read) {
            int res = send(sock, buf + sent, bytes_read - sent, 0);
            if (res <= 0) {
                perror("http_server2: send error");
                fclose(file);
                return -1;
            }
            sent += res;
        }
    }
    
    /* close socket and free pointers */ // no im doing that not in here
    fclose(file);
	return 0;
}



int
main(int argc, char ** argv)
{
    int server_port = -1;
    int ret         =  0;
    int sock        = -1;

    /* parse command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: http_server2 port\n");
        exit(-1);
    }

    server_port = atoi(argv[1]);

    if (server_port < 1500) {
        fprintf(stderr, "Requested port(%d) must be above 1500\n", server_port);
        exit(-1);
    }
    
    /* initialize and make socket */
    // same as tcp_server.c
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("tcp_server: socket error");
        exit(-1);
    }
    /* set server address */
    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(server_port);
    /* bind listening socket */
    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("tcp_server: bind error");
        exit(-1);
    }
    /* start listening */
    if (listen(sock, 32) < 0) {
        perror("tcp_server: listen error");
        exit(-1);
    }
    /* connection handling loop: wait to accept connection */

    int connections[FD_SETSIZE];
    int max_fd; // for select
    int ready; // for select (how many sockets are ready)

    fd_set read_fds, all_fds;

    // initialize connections array to -1 (aka empty)
    for (int i = 0; i < FD_SETSIZE; i++) {
        connections[i] = -1;
    }

    FD_ZERO(&all_fds);
    FD_SET(sock, &all_fds); 
    max_fd = sock;

    while (1) {
    
        /* create read list */
        read_fds = all_fds;
        /*
            ok so the reason we need read_fds and all_fds is because select modifies 
            the set you pass in to indicate which ones are ready
            
            so we keep an unmodified copy in all_fds and copy it to read_fds each time 
            before we call select :D
        */
        
        /* do a select() */
        // resource: https://stackoverflow.com/questions/32711521/how-to-use-select-on-sockets-properly
        // & i did something similar in the client
        ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (ready < 0) {
            perror("http_server2: select error");
            continue;
        }
        /* process sockets that are ready:
         *     for the accept socket, add accepted connection to connections PART 1
         *     for a connection socket, handle the connection PART 2
         */
        
         //PART 1 
        // check if listening socket is ready (aka new connection)
        if (FD_ISSET(sock, &read_fds)) {
            int new_conn = accept(sock, NULL, NULL);

            if (new_conn < 0) {
                perror("http_server2: accept error");
                // exit(-1);
                continue;
            }else{
                //find empty space
                int added = 0;
                for(int i = 0; i < FD_SETSIZE; i++) {
                    if (connections[i] == -1) {
                        connections[i] = new_conn;
                        FD_SET(new_conn, &all_fds);
                        if (new_conn > max_fd) {
                            max_fd = new_conn;
                        }
                        // printf("New connection accepted: %d\n", new_conn);
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    fprintf(stderr, "Too many connections, rejecting new connection\n");
                    close(new_conn);
                }
            }
        }

        // PARRT2
        // check all conn sockets
        for(int i=0; i < FD_SETSIZE; i++) {
            int conn = connections[i];

            if(conn == -1) {
                continue; //empty
            }

            if (FD_ISSET(conn, &read_fds)) {
                // printf("Handling connection: %d\n", conn);
                ret = handle_connection(conn);

                if(ret <0){
                    printf("http_server2: error handling connection\n");
        }

                close(conn);
                FD_CLR(conn, &all_fds); // remove from set
                connections[i] = -1; // mark as empty
            }
        }

        //ret = handle_connection(sock);
        
        //(void)ret;  // DELETE ME
    }
    close(sock);
    return 0;
}
