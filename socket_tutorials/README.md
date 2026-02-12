# CS 1652: Data Communications and Computer Networks

This `tutorials` directory includes basic socket examples in C.

## Building

To build the example programs, simply run:

```
make
```

## Running

To demonstrate basic UDP communication, in one terminal window run (note that
5555 is the port for the server to listen on and can be replaced with any
available port number):

```
./udp_server 5555
```

In another terminal window, run:

```
./udp_client localhost 5555
```

The `udp_client` command says we want to send to a server running on
`localhost` (the same computer where the client runs) listening on port `5555`
(needs to match the port you ran the server program with).

In the client terminal window, type some text, and then hit enter. You should
see printouts in both the server and client windows indicating that the message
was received at the server, and that the client received a response from the
server. The client exits after receiving the response from the server, while
the server continues to wait for additional messages. You can run the client
program again to send another message to the server. When you are done, you can
use `ctrl-c` to exit the server.

Usage for the `tcp_server` and `tcp_client` programs is the same as for the UDP
versions.
