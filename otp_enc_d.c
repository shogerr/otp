#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

// Size of buffer.
#define BUFFER_SIZE 70000

// Encoder type server.
#ifdef DECRYPT
#   define SERVER_TYPE 49
#else
#   define SERVER_TYPE 48 
#endif

struct Net
{
    int port;                   // Port to use.
    int fd;                     // Listening socket file descriptor.
    int _fd;                    // Tx/Rx socket file descriptor.
    int max_conn;               // Max number of connections.
    socklen_t c_n;              // Size of sending socket.
    socklen_t optvalue;         // Option value.
    char* buf;                  // Main data bucket.
    char buf_[10];              // Tiny read buffer.
    int n;                      // Send counter.
    int m;                      // Recieve counter.
    struct sockaddr_in addr_s;  // Socket addresses.
    struct sockaddr_in addr_c;
};

// Custom print error. Sets errno if it has not been set.
void _perror(char* s, int i)
{
    if (!errno && i >= 0)
        errno = i;

    perror(s);
    exit(errno);
}

void _debug(char* s)
{
    char* fn = "debug_output";
    FILE* f;
    if (!(f = fopen(fn, "w+")))
        _perror(NULL, -1);

    fwrite(s, strlen(s), sizeof(char), f);
    fclose(f);
}

// Encode string s with key t.
char* encode(char* s, char *t)
{
    const char* a = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    int i;
    int j;
    int k;

    for (i = 0; i < strlen(s); i++)
    {
        // Adjust the values of each character to 0-27
        // If its a space, catch the space.
        // Then do every other character.
        if (s[i] == 32) j = 26;
        else j = s[i] - 65;
        if (t[i] == 32) k = 26;
        else k = t[i] - 65;

        // Handles both the decryption, and encytion, depending on compiler
        // set definitions.
#ifdef DECRYPT
        s[i] = (j - k) % 27;
        if (s[i] < 0) s[i] += 27;
        // Use character map to get the correct character.
        s[i] = a[s[i]];
#else
        s[i] = a[(j + k) % 27];
#endif
    }

}

// Handle reception of transmission.
void _recv(struct Net* s)
{
    int n;

    // Clear the holding buffer.
    memset(s->buf, 0, BUFFER_SIZE);
    while (strstr(s->buf, "@") == NULL)
    {
        // Clear reading buffer.
        memset(s->buf_, 0, sizeof(s->buf_));
        // Read some bytes.
        n = recv(s->_fd, s->buf_, sizeof(s->buf_) - 1, 0);
        // Concat results.
        strcat(s->buf, s->buf_);
        // Break when done.
        if (n == 0)
            break;
    }

    // Remove terminating null character.
    s->buf[strcspn(s->buf, "@")] = '\0';
}

// Ensure data transmission.
void _send(struct Net* c, char* s)
{
    int n = 0;

    // Contintue to try and send data until all data is sent: use string length.
    for(;;)
    {
        if ((n += send(c->_fd, s + n, strlen(s) - n, 0)) < 0)
            _perror("Socket trouble", 1);
        if (n == strlen(s))
            break;
    }
    // Terminating byte.
    send(c->_fd, "@", 1, 0);
}

int main(int argc, char** argv)
{
    // Data structure for sockets.
    struct Net net;
    // Set values for option and max connect.
    net.optvalue = 1;
    net.max_conn = 5;
    // Place for pid during fork.
    pid_t pid;
    // Message and key strings.
    char* m;
    char* k;

    // Check arguments and throw an error if incorrect count.
    if (argc < 2)
        _perror("Provide a port number", 1);

    // Allocate memory for our buffer, and clear.
    net.buf = malloc(BUFFER_SIZE * sizeof(char));
    memset(net.buf, 0, BUFFER_SIZE);

    // Set up the listen socket.
    // Parse port.
    net.port = atoi(argv[1]);
    // AF_INET family.
    net.addr_s.sin_family = AF_INET;
    // Set port.
    net.addr_s.sin_port = htons(net.port);
    // Any address.
    net.addr_s.sin_addr.s_addr = INADDR_ANY;

    // Create the socket.
    if ((net.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        _perror(NULL, -1);

    setsockopt(net.fd, SOL_SOCKET, SO_REUSEADDR, &net.optvalue, sizeof(socklen_t));

    // Binde socket, throw error otherwise.
    if (bind(net.fd, (struct sockaddr *) &net.addr_s, sizeof(net.addr_s)))
        _perror(NULL, -1);

    // Lisetn for connections.
    listen(net.fd, net.max_conn);

    for (;;)
    {
        net.c_n = sizeof(net.addr_c);
        net._fd = accept(net.fd, (struct sockaddr *) &net.addr_c, &net.c_n);

        if (net._fd < 0)
            _perror(NULL, -1);

        pid = fork();

        // Fork failure check.
        if (pid < 0)
            _perror(NULL, -1);
        // Handle child.
        else if (!pid)
        {
            memset(net.buf, 0, BUFFER_SIZE);

            // Recieve a client qualifier byte.
            net.n = recv(net._fd, net.buf, BUFFER_SIZE-1, 0);

            if (net.n < 0)
                _perror("Error reading from socket", 1);

            // Check the first character for an indications of the correct
            // client.
            if (net.buf[0] == SERVER_TYPE)
                send(net._fd, "0", 1, 0);
            // Reject and close otherwise.
            else
            {
                send(net._fd, "1", 1, 0);
                close(net._fd);
                exit(EXIT_FAILURE);
            }

            // Recieve message. Create heap space, and store string.
            _recv(&net);
            m = calloc(strlen(net.buf), sizeof(char));
            strcpy(m, net.buf);

            // Recieve key.
            _recv(&net);
            k = calloc(strlen(net.buf), sizeof(char));
            strcpy(k, net.buf);

            // Encrypt the message.
            encode(m, k);

            // Send encoding..
            _send(&net, m);

            // Cleanup heap.
            close(net._fd);
            free(m);
            m = 0;
            free(k);
            k = 0;
            exit(EXIT_SUCCESS);
        }
        // Close new socket file handle.
        close(net._fd);
    }
    // Close listening socket handle.
    close(net.fd);
}
