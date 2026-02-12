#include "net_include.h"

#define MAX_LINE 1024

int main(const int argc, const char** argv)
{
    const char        *hostname;
    int                s, port, len, res;
    socklen_t          fromlen;
    char               buf[MAX_LINE];
    struct hostent    *hp;
    struct sockaddr_in saddr, raddr;
    int                from_ip;
    fd_set             read_mask, write_mask, error_mask;
    fd_set             tmp_rmask;
    int                num;

    /* Parse commandline args */
    if (argc < 3 || ((port = atoi(argv[2])) <= 0 || port > 65535)) {
        fprintf(stderr, "usage: udp_client <hostname> <port>\n");
        exit(-1);
    }
    hostname = argv[1];

    /* Open socket */
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("udp_client: failed to create socket");
        exit(-1);
    }

    /* Get IP address from hostname (via DNS lookup) */
    if ((hp = gethostbyname(hostname)) == NULL) {
        herror("udp_client: gethostbyname error");
        exit(-1);
    }

    /* Set up address struct with server IP address and port */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    memcpy(&saddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
    saddr.sin_port = htons(port);

    /* Zero out all masks to initialize */
    FD_ZERO(&read_mask);
    FD_ZERO(&write_mask);
    FD_ZERO(&error_mask);
    /* Set up mask for file descriptors we want to read from */
    FD_SET(s, &read_mask);
    FD_SET((long)0, &read_mask); /* stdin */

    while (1)
    {
        /* (Re)set read mask. The select() call modifies the masks that are
         * passed in to indicate which are ready for operations, so we need to
         * re-init on each iteration. Note that we don't do this for write and
         * error masks just because we're not currently interested in waiting
         * on any write/error fds; otherwise should add tmp_mask versions for
         * them too */
        tmp_rmask = read_mask;

        /* Wait for message (NULL timeout value means wait forever) */
        num = select(FD_SETSIZE, &tmp_rmask, &write_mask, &error_mask, NULL);
        if (num > 0) {
            if (FD_ISSET(s, &tmp_rmask)) {
                /* Receive from server */
                fromlen = sizeof(raddr);

                if ((res = recvfrom(s, buf, sizeof(buf) - 1, 0,
                                    (struct sockaddr *)&raddr,
                                    &fromlen)) < 0) {
                    perror("udp_client: recvfrom error");
                    exit(-1);
                }

                /* Print result */

                /* Adding terminating byte to the message just because we'll be
                 * printing to screen (note that we left space for it) */
                buf[res] = '\0';

                from_ip = ntohl(raddr.sin_addr.s_addr);
                printf("Received %d bytes from %d.%d.%d.%d:%d %s\n",
                       res,
                       (from_ip & 0xff000000)>>24,
                       (from_ip & 0x00ff0000)>>16,
                       (from_ip & 0x0000ff00)>>8,
                       (from_ip & 0x000000ff),
                       ntohs(raddr.sin_port),
                       buf);

            } else if (FD_ISSET(0, &tmp_rmask)) { /* 0 for stdin */
                /* Read from keyboard */
                if (fgets(buf, sizeof(buf), stdin) == NULL)  {
                    fprintf(stderr, "udp_client: fgets error\n");
                    exit(-1);
                }
                len = strlen(buf);

                /* Send to server */
                if ((res = sendto(s, buf, len, 0,
                                  (struct sockaddr *)&saddr,
                                  sizeof(saddr))) < 0) {
                    perror("udp_client: sendto error");
                    exit(-1);
                }
            }
        }
    }

    close(s);
    return 0;
}
