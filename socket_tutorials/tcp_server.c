#include "net_include.h"
#include <ctype.h>

#define MAX_LINE 1024

int main(const int argc, const char** argv)
{
    int i, s, c, len, res, port;
    char buf[MAX_LINE];
    struct sockaddr_in saddr;

    /* Parse commandline args */
    if (argc < 2 || ((port = atoi(argv[1])) <= 0 || port > 65535)) {
       fprintf(stderr, "usage: tcp_server <port>\n");
       exit(-1);
    }

    /* Open socket */
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("tcp_server: socket error");
        exit(-1);
    }

    /* Set up address to bind to */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    /* Bind to receive messages on specified port */
    if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("tcp_server: bind error");
        exit(-1);
    }

    /* Listen for connection attempts on this socket (32 = max queued
     * connections) */
    if (listen(s, 32) < 0) {
        perror("tcp_server: listen error");
        exit(-1);
    }

    while ((c = accept(s, NULL, NULL)) >= 0) {

        /* Read data from newly created socket c */
        /*if ((len = read(c, buf, sizeof(buf) - 1)) <= 0)  {*/
        if ((len = recv(c, buf, sizeof(buf) - 1, 0)) <= 0)  {
            perror("tcp_server: recv error");
            exit(-1);
        }

        /* Print out received message (just for illustration) */
        buf[len] = '\0';
        printf("Received %d bytes: %s\n", len, buf);

        /* Convert received message to upper case */
        for (i = 0; i < len; i++) {
            if (islower(buf[i])) {
                buf[i] = toupper(buf[i]);
            }
        }

        /* Send modified message to client */
        /*if ((res = write(c, buf, len)) <= 0) {*/
        if ((res = send(c, buf, len, 0)) <= 0) {
            perror("tcp_server: send error");
            exit(-1);
        }

        close(c);
    }

    close(s);
    return(0);
}
