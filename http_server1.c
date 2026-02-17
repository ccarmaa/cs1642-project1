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

    char * ok_response_f  = "HTTP/1.0 200 OK\r\n"        \
        					"Content-type: text/plain\r\n"                  \
        					"Content-length: %d \r\n\r\n";
 
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"   \
        					"Content-type: text/html\r\n\r\n"                       \
        					"<html><body bgColor=black text=white>\n"               \
        					"<h2>404 FILE NOT FOUND</h2>\n"
        					"</body></html>\n";
    
	//(void)notok_response;  // DELETE ME
	//(void)ok_response_f;   // DELETE ME

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
                perror("http_server1: send error");
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

    /* parse command line args */ // GIVEN
    if (argc != 2) {
        fprintf(stderr, "usage: http_server1 port\n");
        exit(-1);
    }

    server_port = atoi(argv[1]); //GIVEN

    if (server_port < 1500) {  //GIVEN
        fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
        exit(-1);
    }

    /* initialize and make socket */
    // same as tcp_server.c
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("http_server1: socket error");
        exit(-1);
    }

    /* set server address */
    // same as tcp_server.c
    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(server_port);

    /* bind listening socket */
    // same as tcp_server.c
    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("http_server1: bind error");
        exit(-1);
    }

    /* start listening */
    // same as tcp_server.c
    // tbh im assuming we are going with 32 max connections again who knows
    if (listen(sock, 32) < 0) {
        perror("http_server1: listen error");
        exit(-1);
    }
    int c;
    /* connection handling loop: wait to accept connection */
    while (1) {
        // first we want to accept the connection
        // this is gonna be slightly different from tcp_seveer.c
        // so ig we break if < 0 instead of the while thingy
        if((c = accept(sock, NULL, NULL)) < 0) {
            perror("http_server1: accept error");
            continue;
        }
        



        /* handle connections */
        ret = handle_connection(c);

		if(ret <0){
            printf("http_server1: error handling connection\n");
        }
        

        close(c);
    }

    close(sock);
    return 0;
}
