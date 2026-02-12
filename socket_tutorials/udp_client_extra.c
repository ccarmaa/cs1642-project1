#include "net_include.h" 
#include <arpa/inet.h> /* for inet_ntop() */

static void Usage(int argc, char *argv[]);
static void Print_help(void);
static void Print_IP(const struct sockaddr *sa);
static void Print_IP_Manual(const struct sockaddr *sa);

static char *Server_IP;
static char *Port_Str;

int main(int argc, char *argv[])
{
    int                     sock;
    struct addrinfo         hints, *servinfo, *servaddr;
    struct sockaddr_storage from_addr;
    socklen_t               from_len;
    fd_set                  mask, read_mask;
    int                     bytes, num, ret;
    char                    mess_buf[MAX_MESS_LEN];
    char                    input_buf[MAX_MESS_LEN];

    /* Parse commandline args */
    Usage(argc, argv);
    printf("Sending to %s at port %s\n", Server_IP, Port_Str);

    /* Set up hints to use with getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* we'll use AF_INET for IPv4, but can use AF_INET6 for IPv6 or AF_UNSPEC if either is ok */
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    /* Use getaddrinfo to get IP info for string IP address (or hostname) in
     * correct format */
    ret = getaddrinfo(Server_IP, Port_Str, &hints, &servinfo);
    if (ret != 0) {
       fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(ret));
       exit(1);
    }

    /* Loop over list of available addresses and take the first one that works
     * */
    for (servaddr = servinfo; servaddr != NULL; servaddr = servaddr->ai_next) {
        /* print IP, just for demonstration */
        printf("Found server address:\n");
        Print_IP(servaddr->ai_addr);
        Print_IP_Manual(servaddr->ai_addr);
        printf("\n");

        /* setup socket based on addr info. manual setup would look like:
         *   socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) */
        sock = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol);
        if (sock < 0) {
            perror("udp_client: socket");
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
    FD_SET((long)0, &read_mask); /* 0 == stdin */

    for(;;)
    {
        /* (Re)set mask */
        mask = read_mask;

        /* Wait for message (NULL timeout = wait forever) */
        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0) {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len);

                Print_IP((struct sockaddr *)&from_addr);
                printf("Received message: %s\n", mess_buf);

            } else if (FD_ISSET(0, &mask)) {
                bytes = read(0, input_buf, sizeof(input_buf)-1);

                /* Adding terminating byte to the message just because we'll be
                 * printing to screen (note that we left space for it) */
                input_buf[bytes] = '\0';
                bytes++;

                printf("Read input from stdin: %s\n", input_buf);

                sendto(sock, input_buf, bytes, 0, servaddr->ai_addr,
                       servaddr->ai_addrlen);
            }
        }
    }

    /* Cleanup */
    freeaddrinfo(servinfo);
    close(sock);

    return 0;

}

/* Print an IP address from sockaddr struct, using API functions */
void Print_IP(const struct sockaddr *sa)
{
    char ipstr[INET6_ADDRSTRLEN];
    void *addr;
    char *ipver;
    uint16_t port;

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
        addr = &(ipv4->sin_addr);
        port = ntohs(ipv4->sin_port);
        ipver = "IPv4";
    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
        addr = &(ipv6->sin6_addr);
        port = ntohs(ipv6->sin6_port);
        ipver = "IPv6";
    } else {
        printf("Unknown address family\n");
        return;
    }

    inet_ntop(sa->sa_family, addr, ipstr, sizeof(ipstr));
    printf("%s: %s:%d\n", ipver, ipstr, port);
}

/* Print an IP address from sockaddr struct by manipulating address manually */
void Print_IP_Manual(const struct sockaddr *sa)
{
    uint8_t *addr;

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;

        /* Grab IP (in network byte order) from sockaddr_in struct */
        uint32_t from_ip_net = ipv4->sin_addr.s_addr;

        /* Convert from network to host byte order */
        uint32_t from_ip = ntohl(from_ip_net);

        /* Print each octet of the IP address */
        printf("IPv4: %d.%d.%d.%d\n", 
                   (from_ip & 0xff000000)>>24,
                   (from_ip & 0x00ff0000)>>16,
                   (from_ip & 0x0000ff00)>>8,
                   (from_ip & 0x000000ff));

        /* Alternative method */
        addr = (uint8_t *)&from_ip_net;
        printf("IPv4: %d.%d.%d.%d\n", addr[0], addr[1], addr[2], addr[3]);
    } else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;

        addr = (uint8_t *)&(ipv6->sin6_addr.s6_addr);

        printf("IPv6: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
                   (int)addr[0],  (int)addr[1],
                   (int)addr[2],  (int)addr[3],
                   (int)addr[4],  (int)addr[5],
                   (int)addr[6],  (int)addr[7],
                   (int)addr[8],  (int)addr[9],
                   (int)addr[10], (int)addr[11],
                   (int)addr[12], (int)addr[13],
                   (int)addr[14], (int)addr[15]);
    }

}
/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    char *port_delim;

    if (argc != 2) {
        Print_help();
    }

    /* Find ':' separating IP and port, and zero it */
    port_delim = strrchr(argv[1], ':');
    if (port_delim == NULL) {
        fprintf(stderr, "Error: invalid IP/port format\n");
        Print_help();
    }   
    *port_delim = '\0';

    Port_Str = port_delim + 1;

    Server_IP = argv[1];
    /* allow IPv6 format like: [::1]:5555 by striping [] from Server_IP */
    if (Server_IP[0] == '[') {
        Server_IP++;
        Server_IP[strlen(Server_IP) - 1] = '\0';
    }
}

static void Print_help(void) {
    printf("Usage: udp_client <server_ip>:<port>\n");
    exit(0);
}
