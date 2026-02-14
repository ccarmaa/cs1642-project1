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
    
//   (void)notok_response;  // DELETE ME
//   (void)ok_response_f;   // DELETE ME

    /* first read loop -- get request and headers*/
    ///
    int len;
    char buf[BUFSIZE];
    if ((len = recv(sock, buf, sizeof(buf) - 1, 0)) <= 0)  {
        perror("http_server2: recv error");
        close(sock);
        return -1; // exit(-1)
    }

    /* parse request to get file name */
    /* Assumption: For this project you only need to handle GET requests and filenames that contain no spaces */
    ///
    char file_name[FILENAMESIZE];
    sscanf(buf, "GET %s HTTP", file_name);
    
/// do we need this?
    if (file_name[0] == '/') {
        memmove(file_name, file_name + 1, strlen(file_name));
    }

    /* open and read the file */
    ///
    FILE *fptr;
    fptr = fopen(file_name, "r");
    if (fptr == NULL) {
        send(sock, notok_response, strlen(notok_response), 0);
        close(sock);
        return -1;
    }
  
    /* send response */
    ///
    fseek(fptr, 0, SEEK_END);
    long file_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    char header[BUFSIZE];
    snprintf(header, BUFSIZE, ok_response_f, file_size);

    send(sock, header, strlen(header), 0);

    int bytes_read;
    while ((bytes_read = fread(buf, 1, BUFSIZE, fptr)) > 0) {
        int sent = 0;
        while (sent < bytes_read) {
            int res = send(sock, buf + sent, bytes_read - sent, 0);
            if (res <= 0) {
                perror("http_server2: send error");
                close(sock);
                fclose(fptr);
                return -1;
            }
            sent += res;
        }
    }

    /* close socket and free space */
    ///
    close(sock);
    fclose(fptr);
  
    return 0;
}



int
main(int argc, char ** argv)
{
    int server_port = -1;
    int ret         =  0;
    int sock        = -1;

    ///
    int s;
    struct sockaddr_in saddr;
    int connections[FD_SETSIZE];
    ///

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
    ///
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) { 
        perror("http_server2: socket error");
        exit(-1);
    }

    /* set server address */
    ///
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(server_port);

    /* bind listening socket */
    ///
    if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("http_server2: bind error");
        exit(-1);
    }

    /* start listening */
    ///
    if (listen(s, 32) < 0) {
        perror("http_server2: listen error");
        exit(-1);
    }

    /// init to -1
    for (int i = 0; i < FD_SETSIZE; i++) {
        connections[i] = -1;
    }

    /* connection handling loop: wait to accept connection */

    while (1) {
    
        /* create read list */
        ///
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(s, &fds);
        int max_fd = s;

        // add open connections
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (connections[i] != -1) {
                FD_SET(connections[i], &fds);
                if (connections[i] > max_fd) {
                    max_fd = connections[i];
                }
            }
        }
        
        /* do a select() */
        ///
        int num_ready = select(max_fd + 1, &fds, NULL, NULL, NULL);
        if (num_ready <= 0) {
            perror("http_server2: select error");
            continue;
        }
        
        /* process sockets that are ready:
         *     for the accept socket, add accepted connection to connections
         *     for a connection socket, handle the connection
         */

        ///
        if (FD_ISSET(s, &fds)) {
            sock = accept(s, NULL, NULL);
            if (sock < 0) {
                perror("http_server2: accept error");
            } else {
                // add to connections array
                int added = 0;
                for (int i = 0; i < FD_SETSIZE; i++) {
                    if (connections[i] == -1) {
                        connections[i] = sock;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    fprintf(stderr, "http_server2: too many connections, rejecting\n");
                    close(sock);
                }
            }
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (connections[i] != -1 && FD_ISSET(connections[i], &fds)) {
                ret = handle_connection(connections[i]);
                (void)ret;
                
                /// remove from connections array 
                /// socket closed in handle_connection
                connections[i] = -1;
            }
        }
        
        // ret = handle_connection(sock);
        
        // (void)ret;  // DELETE ME
    }
}
