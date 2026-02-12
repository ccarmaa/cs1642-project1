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

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 1024

int 
main(int argc, char ** argv) 
{

    char * server_name = NULL;
    int    server_port = -1;
    char * server_path = NULL;
    char * req_str     = NULL;

    //
    int s, len, res;
    char buf[BUFSIZE];
    struct hostent *hp;
    struct sockaddr_in saddr;

    int ret = 0;

    /*parse args */
    if (argc != 4) {
        fprintf(stderr, "usage: http_client <hostname> <port> <path>\n");
        exit(-1);
    }

    server_name = argv[1];
    server_port = atoi(argv[2]);
    server_path = argv[3];
    
    /* Create HTTP request */
    ret = asprintf(&req_str, "GET %s HTTP/1.0\r\n\r\n", server_path);
    if (ret == -1) {
        fprintf(stderr, "Failed to allocate request string\n");
        exit(-1);
    }

    /*
     * NULL accesses to avoid compiler warnings about unused variables
     * You should delete the following lines 
     */
    // (void)server_name;
    // (void)server_port;

    /* make socket */
    // int s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("tcp_client: failed to create socket");
        exit(-1);
    }

    /* get host IP address  */
    /* Hint: use gethostbyname() */
    if ((hp = gethostbyname(server_name)) == NULL) {
        herror("tcp_client: gethostbyname error");
        exit(-1);
    }

    /* set address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    memcpy(&saddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
    saddr.sin_port = htons(server_port);

    /* connect to the server */
    if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("tcp_client: could not connect to server");
        exit(-1);
    }
    // printf("Connected!\n");

    /* send request message */
    /* IMPORTANT NOTE: send/write is NOT guaranteed to send the entire message
     * in a single call (may have res < len). What should you do to handle this
     * case? */

    // on success, these calls return the number of bytes sent 
    // on error, -1 is returned, and errno is set to indicate the error
    len = strlen(req_str);
    // we want the returned number of bytes to equal the sent number of bytes
    // the sent number of bytes is equal to len = strlen(req_str)
    int sent = 0;

    while (sent < len) {
        res = send(s, req_str + sent, len - sent, 0);
        if (res <= 0) {
            perror("http_client: send error");
            close(s);
            exit(-1);
        }
        sent += res;
    }

    /* wait for response (i.e. wait until socket can be read) */
    /* Hint: use select(), and ignore timeout for now. */
    fd_set fds; // fd set
    FD_ZERO(&fds); // initialize to empty
    FD_SET(s, &fds); // add socket s to the set

    // blocks (waits) until one of the file descriptors in fds is ready to read
    // returns number of fds ready to read
    int num = select(s + 1, &fds, NULL, NULL, NULL); // 3 NULLs are socket writing, socket errors, timeout
    if (num <= 0) {
        perror("http_client: select error");
        close(s);
        exit(-1);
    }

    /* IMPORTANT NOTE: recv/read is NOT guaranteed to send the entire message
     * in a single call. What should you do to handle this case? */

    // continue calling recv until it's <= 0?

    /* first read loop -- read headers */
    /* examine return code */   
    // Skip protocol version (e.g. "HTTP/1.0")
    // Normal reply has return code 200

    res = recv(s, buf, sizeof(buf) - 1, 0);
    if (res <= 0) {
        perror("http_client: recv error");
        exit(-1);
    }
    buf[res] = '\0';

    char *body_start = strstr(buf, "\r\n\r\n"); // where headers end
    if (body_start == NULL) {
        fprintf(stderr, "http_client: invalid HTTP response - no header delimiter\n");
        close(s);
        return -1;
    }
    *body_start = '\0'; // null-terminate end of headers

    int status_code;
    sscanf(buf, "HTTP/%*s %d", &status_code);

    if (status_code == 200) { // no error
        *body_start = '\r'; // replace null-terminator with original '\r'
        body_start += 4; // move to start of body

        printf("%s", body_start); // print what we have so far

        /* second read loop -- print out the rest of the response: real web content */
        // keep receiving 
        while ((res = recv(s, buf, BUFSIZE - 1, 0)) > 0) {
            buf[res] = '\0';
            printf("%s", buf);
        }

        // check for errors
        if (res < 0) {
            perror("http_client: recv error");
            close(s);
            return -1;
        }

        close(s);
        return 0;
    }
    else { // error
        *body_start = '\r'; // replace null-terminator with original '\r'

        /* print first part of response: header, error code, etc. */
        fprintf(stderr, "%s", buf);

        // keep receiving 
        while ((res = recv(s, buf, BUFSIZE - 1, 0)) > 0) {
            buf[res] = '\0';
            fprintf(stderr, "%s", buf);
        }

        close(s);
        return -1;
    }

    /* close socket */
    // close(s);
    // return 0;
}