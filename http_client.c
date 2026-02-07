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

int main(int argc, char **argv)
{

    char *server_name = NULL;
    int server_port = -1;
    char *server_path = NULL;
    char *req_str = NULL;

    int ret = 0;

    /*parse args */
    if (argc != 4)
    {
        fprintf(stderr, "usage: http_client <hostname> <port> <path>\n");
        exit(-1);
    }

    server_name = argv[1];
    server_port = atoi(argv[2]);
    server_path = argv[3];

    /* Create HTTP request */
    ret = asprintf(&req_str, "GET %s HTTP/1.0\r\n\r\n", server_path);
    if (ret == -1)
    {
        fprintf(stderr, "Failed to allocate request string\n");
        exit(-1);
    }

    /*
     * NULL accesses to avoid compiler warnings about unused variables
     * You should delete the following lines
     */

    /*90% OF THE WORK HERE IS LITERALLY JUST THE TCP_CLIENT.C FILE FROM THE SOCKET EXAMPLE STUFF <3 */

    /* make socket */
    /*
        ok so socket(....) returns
        -1 on error, and a non-negative integer (the file descriptor) on success.
        we can use s to read/write to the socket and close(s)
    */
    int s;
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("tcp_client: failed to create socket");
        exit(-1);
    }

    /* get host IP address  */
    /* Hint: use gethostbyname() */
    /*
        this returns a struct hostent* on success and null on failure
        this contains ip address of server... hrm
    */
    struct hostent *hp;
    if ((hp = gethostbyname(server_name)) == NULL)
    {
        herror("tcp_client: gethostbyname error");
        exit(-1);
    }

    /* set address */
    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(saddr));                         // zero out struct
    saddr.sin_family = AF_INET;                               // set family to af_inet
    memcpy(&saddr.sin_addr.s_addr, hp->h_addr, hp->h_length); // copy ip address from hostent -> sockaddr_in
    saddr.sin_port = htons(server_port);                      // set port number

    /* connect to the server */
    /*
        connect() returns 0 on success and -1 on failure
        args of connect are (int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    */
    if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("tcp_client: could not connect to server");
        exit(-1);
    }

    /* send request message */
    /*
        ok i think we r requesting req_str (i think its formatted correctly)
        send() returns the number of bytes sent on success and -1 on failure

        replaced buf with req_str and len with strlen(req_str) from above
    */

    printf("REQ STRING: ");
    printf("%s", req_str);
    int len = strlen(req_str);
    int res;
    if ((res = send(s, req_str, len, 0)) <= 0)
    {
        perror("tcp_client: send error");
        exit(-1);
    }

    // i fear i may no longer be able to copy over code...

    /* wait for response (i.e. wait until socket can be read) */
    /* Hint: use select(), and ignore timeout for now. */

    /*
    resources:
    https://man7.org/linux/man-pages/man2/select.2.html
    no one on stack overflow was helpful. they were all more confused then me so i had to read the man:(

    notes:
    - args of select are (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
        - i think we only care about readfds for now and we can ignore timeout (??)
        - nfds = max(readfds, writefds, exceptfds) + 1 (which should just be s+1 in this case)
        - readfds = set of file descriptors to read from (just s in this case)
        - timeout = null to ignore timeout
    */

    // ok! the man pages say to first clear the set, then add to it :)
    fd_set readfds;
    FD_ZERO(&readfds);   // clear
    FD_SET(s, &readfds); // add

    // WAIT FOR RESPONSE (finally)
    int select_ret = select(s + 1, &readfds, NULL, NULL, NULL);
    if (select_ret < 0)
    {
        perror("select error");
        exit(-1);
    }

    char buf[BUFSIZE];               // buffsize defined at top
    char headers[BUFSIZE * 5] = {0}; // to store headers... not sure if necessary tbh

    /* first read loop -- read headers */
    // loop :)
    // ok so we are looping bc header may come in multiple pieces
    while (1)
    {
        /* read from socket into buf */
        /*
            recv() ret # of bytes on return, 0 if conn closed, -1 on error
            args are (int sockfd, void *buf, size_t len, int flags);
        */
        res = recv(s, buf, sizeof(buf) - 1, 0);

        /* Hint: use recv() instead of read() */
        /* examine return code */
        // check return codes ^^
        if (res < 0)
        {
            perror("recv error");
            exit(-1);
        }
        else if (res == 0)
        {
            fprintf(stderr, "server closed connection\n");
            break;
        }
        // null term at edn so we can print it
        buf[res] = '\0';
        strcat(headers, buf); // add to headers

        /* if we have received the full header (i.e. we see \r\n\r\n), break out of loop */
        if (strstr(headers, "\r\n\r\n") != NULL)
        { // strstr finds substring in string
            break;
        }
    }

    /* examine return code */
    // Skip protocol version (e.g. "HTTP/1.0")
    // Normal reply has return code 200

    /* print first part of response: header, error code, etc. */
    int status_code = 0;
    sscanf(headers, "HTTP/%*s %d", &status_code);
    printf("Status Code: %d\n\n", status_code);
    printf("%s\n", headers);

    /* second read loop -- print out the rest of the response: real web content */
    while ((res = recv(s, buf, BUFSIZE - 1, 0)) > 0)
    {
        buf[res] = '\0';
        printf("%s", buf);
    }

    /* close socket */
    close(s);
}
