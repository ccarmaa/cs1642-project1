#include "net_include.h"
#include <ctype.h>

#define MAX_LINE 1024

int main(const int argc, const char** argv)
{
    int                i, s, len, res, port;
    socklen_t          fromlen;
    char               buf[MAX_LINE];
    struct sockaddr_in saddr, claddr;
    int                from_ip;

    /* Parse commandline args */
    if (argc < 2 || ((port = atoi(argv[1])) <= 0 || port > 65535)) {
        fprintf(stderr, "usage: udp_server <port>\n");
        exit(-1);
    }

    /* Open socket */
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("udp_server: failed to create socket");
        exit(-1);
    }

    /* Set up addr struct with port to bind to */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    /* Bind socket to receive messages on specified port */
    if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("udp_server: bind");
        exit(-1);
    }

    while (1)
    {
        /* Receive datagram from client */
        fromlen = sizeof(claddr); 

        if ((len = recvfrom(s, buf, sizeof(buf) - 1, 0,
                            (struct sockaddr *)&claddr, 
                            &fromlen)) <= 0) {
            perror("udp_server: recvfrom error"); 
            exit(-1);
        }

        /* Print out received message (just for illustration) */
        buf[len] = '\0';
        from_ip = ntohl(claddr.sin_addr.s_addr);
        printf("Received %d bytes from %d.%d.%d.%d:%d %s\n",
               len,
               (from_ip & 0xff000000)>>24,
               (from_ip & 0x00ff0000)>>16,
               (from_ip & 0x0000ff00)>>8,
               (from_ip & 0x000000ff),
               ntohs(claddr.sin_port),
               buf);

        /* Convert received message to upper case */
        for (i = 0; i < len; i++) {
            if (islower(buf[i])) {
               buf[i] = toupper(buf[i]);
            }
        }

        /* Send modified message to client (using address claddr filled in by
         * recvfrom call) */
        if ((res = sendto(s, buf, len, 0, 
                          (struct sockaddr *)&claddr, 
                          fromlen)) <= 0) {
            perror("udp_server: sendto error"); 
            exit(-1);
        }
    }
  
    close(s);
    return(0);
} 

