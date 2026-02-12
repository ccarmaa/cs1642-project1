#include "net_include.h"

#define MAX_LINE 1024

int main(const int argc, const char** argv)
{
    const char *hostname;
    int s, port, len, res;
    char buf[MAX_LINE];
    struct hostent *hp;
    struct sockaddr_in saddr;

    /* Parse commandline args */
    if (argc < 3 || ((port = atoi(argv[2])) <= 0 || port > 65535)){
        fprintf(stderr, "usage: tcp_client <hostname> <port>\n");
        exit(-1);
    }
    hostname = argv[1];

    /* Open socket */
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("tcp_client: failed to create socket");
        exit(-1);
    }

    /* Get IP address from hostname (via DNS lookup) */
    if ((hp = gethostbyname(hostname)) == NULL) {
        herror("tcp_client: gethostbyname error");
        exit(-1);
    }

    /* Set up address struct with server IP address and port */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    memcpy(&saddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
    saddr.sin_port = htons(port);

    /* Connect to server */
    if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("tcp_client: could not connect to server");
        exit(-1);
    }
    printf("Connected!\n");

    /* Read from keyboard */
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        fprintf(stderr, "tcp_client: fgets error\n");
        exit(-1);
    }
    len = strlen(buf);

    /* Send to server */
    /* IMPORTANT NOTE: send/write is NOT guaranteed to send the entire message
     * in a single call (may have res < len). What should you do to handle this
     * case? */
    /*if ((res = write(s, buf, len)) <= 0) {*/
    if ((res = send(s, buf, len, 0)) <= 0) {
        perror("tcp_client: send error");
        exit(-1);
    }

    /* Receive from server */
    /* IMPORTANT NOTE: recv/read is NOT guaranteed to send the entire message
     * in a single call. What should you do to handle this case? */
    /*if ((res = read(s, buf, sizeof(buf)-1)) <= 0) {*/
    if ((res = recv(s, buf, sizeof(buf) - 1, 0)) <= 0) {
        perror("tcp_client: recv error");
        exit(-1);
    }

    /* Print result */
    buf[res] = '\0'; /* ensure string termination for printing */
    printf("Received %d bytes: %s\n", res, buf);

    close(s);
    return 0;
}
