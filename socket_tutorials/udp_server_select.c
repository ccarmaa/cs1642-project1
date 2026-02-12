#include "net_include.h"
#include <ctype.h>

static int Cmp_time(struct timeval t1, struct timeval t2);

#define MAX_LINE 1024

static const struct timeval Zero_time = {0, 0};
static const struct timeval Timeout = {10, 0};

int main(const int argc, const char** argv)
{
    int                i, s, len, res, port;
    socklen_t          fromlen;
    char               buf[MAX_LINE];
    struct sockaddr_in saddr, claddr;
    int                from_ip;
    fd_set             read_mask, write_mask, error_mask;
    fd_set             tmp_rmask;
    int                num;
    struct timeval     tmp_timeout, now, diff_time;
    struct timeval     last_recv_time = Zero_time;

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

    /* Zero out all masks to initialize */
    FD_ZERO(&read_mask);
    FD_ZERO(&write_mask);
    FD_ZERO(&error_mask);
    /* Set up mask for file descriptors we want to read from */
    FD_SET(s, &read_mask);

    while (1)
    {
        /* Reset read mask and timeout */
        tmp_rmask = read_mask;
        tmp_timeout = Timeout;

        num = select(FD_SETSIZE, &tmp_rmask, &write_mask, &error_mask, &tmp_timeout);
        if (num > 0) {
            if (FD_ISSET(s, &tmp_rmask)) {
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

                /* Record time */
                gettimeofday(&last_recv_time, NULL);
            }
        } else { /* timeout */
            printf("timeout...nothing received for 10 seconds.\n");
            gettimeofday(&now, NULL);
            if (Cmp_time(last_recv_time, Zero_time) > 0) {
                timersub(&now, &last_recv_time, &diff_time);
                printf("last msg received %lf seconds ago.\n\n",
                        diff_time.tv_sec + (diff_time.tv_usec / 1000000.0));
            }
        }
    }
  
    close(s);
    return(0);
} 

/* Returns 1 if t1 > t2, -1 if t1 < t2, 0 if equal */
static int Cmp_time(struct timeval t1, struct timeval t2) {
    if      (t1.tv_sec  > t2.tv_sec) return 1;
    else if (t1.tv_sec  < t2.tv_sec) return -1;
    else if (t1.tv_usec > t2.tv_usec) return 1;
    else if (t1.tv_usec < t2.tv_usec) return -1;
    else return 0;
}
