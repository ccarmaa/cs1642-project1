#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help(void);
static int Cmp_time(struct timeval t1, struct timeval t2);

static const struct timeval Zero_time = {0, 0};

static char *Port_Str;

int main(int argc, char *argv[])
{
    struct addrinfo         hints, *servinfo, *servaddr;
    struct sockaddr_storage from_addr;
    socklen_t               from_len;
    int                     sock;
    fd_set                  mask, read_mask;
    int                     bytes, num, ret;
    char                    mess_buf[MAX_MESS_LEN];
    struct timeval          timeout;
    struct timeval          last_recv_time = {0, 0};
    struct timeval          now;
    struct timeval          diff_time;
    char                    hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    /* Parse commandline args */
    Usage(argc, argv);
    printf("Listening for messages on port %s\n", Port_Str);

    /* Set up hints to use with getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* we'll use AF_INET for IPv4, but can use AF_INET6 for IPv6 or AF_UNSPEC if either is ok */
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; /* indicates that I want to get my own IP address */

    /* Use getaddrinfo to get list of my own IP addresses */
    ret = getaddrinfo(NULL, Port_Str, &hints, &servinfo);
    if (ret != 0) {
       fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(ret));
       exit(1);
    }

    /* Loop over list of available addresses and take the first one that works
     * */
    for (servaddr = servinfo; servaddr != NULL; servaddr = servaddr->ai_next) {
        /* print IP, just for demonstration */
        ret = getnameinfo(servaddr->ai_addr, servaddr->ai_addrlen, hbuf,
                sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST |
                NI_NUMERICSERV);
        if (ret != 0) {
            fprintf(stderr, "getnameinfo error: %s\n", gai_strerror(ret));
            exit(1);
        }
        printf("Got my IP address: %s:%s\n\n", hbuf, sbuf);

        /* setup socket based on addr info. manual setup would look like:
         *   socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) */
        sock = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol);
        if (sock < 0) {
            perror("udp_server: socket");
            continue;
        }

        /* bind to receive incoming messages on this socket */
        if (bind(sock, servaddr->ai_addr, servaddr->ai_addrlen) < 0) {
            close(sock);
            perror("udp_server: bind");
            continue;
        }
        
        break; /* got a valid socket */
    }

    if (servaddr == NULL) {
        fprintf(stderr, "No valid address found...exiting\n");
        exit(1);
    }

    /* Set up mask for file descriptors we want to read from */
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    for(;;)
    {
        /* (Re)set mask and timeout */
        mask = read_mask;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if (num > 0) {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len);

                /* Record time we received this msg */
                gettimeofday(&last_recv_time, NULL);

                /* print IP received from */
                ret = getnameinfo((struct sockaddr *)&from_addr, from_len, hbuf,
                        sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST |
                        NI_NUMERICSERV);
                if (ret != 0) {
                    fprintf(stderr, "getnameinfo error: %s\n", gai_strerror(ret));
                    exit(1);
                }
                printf("Received from %s:%s: %s\n", hbuf, sbuf, mess_buf);

                /* Echo message back to sender */
                sendto(sock, mess_buf, bytes, 0, (struct sockaddr *)&from_addr,
                       from_len);

            }
        } else {
            printf("timeout...nothing received for 10 seconds.\n");
            gettimeofday(&now, NULL);
            if (Cmp_time(last_recv_time, Zero_time) > 0) {
                timersub(&now, &last_recv_time, &diff_time);
                printf("last msg received %lf seconds ago.\n\n",
                        diff_time.tv_sec + (diff_time.tv_usec / 1000000.0));
            }
        }
    }

    return 0;

}

/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    if (argc != 2) {
        Print_help();
    }

    Port_Str = argv[1];
}

static void Print_help(void) {
    printf("Usage: udp_server <port>\n");
    exit(0);
}

/* Returns 1 if t1 > t2, -1 if t1 < t2, 0 if equal */
static int Cmp_time(struct timeval t1, struct timeval t2) {
    if      (t1.tv_sec  > t2.tv_sec) return 1;
    else if (t1.tv_sec  < t2.tv_sec) return -1;
    else if (t1.tv_usec > t2.tv_usec) return 1;
    else if (t1.tv_usec < t2.tv_usec) return -1;
    else return 0;
}
