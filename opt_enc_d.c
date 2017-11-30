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
    int port;
    int fd;
    int _fd;
    int max_conn;
    socklen_t c_n;
    socklen_t optvalue;
    char* buf;
    char buf_[10];
    int n;
    int m;
    struct sockaddr_in addr_s;
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

char* encode(char* s, char *t)
{
    const char* a = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    int i;
    int j;
    int k;

    for (i = 0; i < strlen(s); i++)
    {
        if (s[i] == 32) j = 26;
        else j = s[i] - 65;
        if (t[i] == 32) k = 26;
        else k = t[i] - 65;

#ifdef DECRYPT
        s[i] = (j - k) % 27;
        if (s[i] < 0) s[i] += 27;
        s[i] = a[s[i]];
#else
        s[i] = a[(j + k) % 27];
#endif
    }

}



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

    s->buf[strcspn(s->buf, "@")] = '\0';
}

void _send(struct Net* c, char* s)
{
    int n = 0;

    for(;;)
    {
        if ((n += send(c->_fd, s + n, strlen(s) - n, 0)) < 0)
            _perror("Socket trouble", 1);
        if (n == strlen(s))
            break;
    }
    send(c->_fd, "@", 1, 0);
}

int main(int argc, char** argv)
{
    struct Net net;
    net.optvalue = 1;
    net.max_conn = 5;
    pid_t pid;
    char* m;
    char* k;

    // Check arguments and throw an error if incorrect count.
    if (argc < 2)
    {
        errno = 1;
        _perror("Provide a port number", 1);
    }

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

    if ((net.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        _perror(NULL, -1);

    setsockopt(net.fd, SOL_SOCKET, SO_REUSEADDR, &net.optvalue, sizeof(socklen_t));

    if (bind(net.fd, (struct sockaddr *) &net.addr_s, sizeof(net.addr_s)))
        _perror(NULL, -1);

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

            net.n = recv(net._fd, net.buf, BUFFER_SIZE-1, 0);

            if (net.n < 0)
            {
                errno = 1;
                _perror("Error reading from socket", 1);
            }

            // Check the first character for an indications of the correct
            // client.
            if (net.buf[0] == SERVER_TYPE)
                send(net._fd, "0", 1, 0);
            else
            {
                send(net._fd, "1", 1, 0);
                close(net._fd);
                exit(EXIT_FAILURE);
            }

            // Recieve message.
            _recv(&net);
            m = calloc(strlen(net.buf), sizeof(char));
            strcpy(m, net.buf);

            // Recieve key.
            _recv(&net);
            k = calloc(strlen(net.buf), sizeof(char));
            strcpy(k, net.buf);

            //_debug(m);
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
