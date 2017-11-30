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
#define SERVER_TYPE 48 

struct Net
{
    int port;
    int fd;
    int _fd;
    int max_conn;
    socklen_t c_n;
    socklen_t optvalue;
    char* buf;
    char* buf_;
    int buf_n;
    int buf_m;
    struct sockaddr_in addr_s;
    struct sockaddr_in addr_c;
};

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

        s[i] = a[(j + k) % 27];
    }

}

void _perror(char* s)
{
    perror(s);
    exit(errno);
}

void _debug(char* s)
{
    char* fn = "debug_output";
    FILE* f;
    if (!(f = fopen(fn, "w+")))
        _perror(NULL);

    fwrite(s, strlen(s), sizeof(char), f);
    fclose(f);
}

int main(int argc, char** argv)
{
    int status;
    int wpid;
    struct Net net;
    net.optvalue = 1;
    net.max_conn = 5;
    pid_t pid;
    char* m;
    char* k;

    char num[10];

    if (argc < 2)
    {
        errno = 1;
        _perror("Provide a port number");
    }

    net.buf = malloc(BUFFER_SIZE * sizeof(char));
    memset(net.buf, 0, BUFFER_SIZE);

    net.port = atoi(argv[1]);
    net.addr_s.sin_family = AF_INET;
    net.addr_s.sin_port = htons(net.port);
    net.addr_s.sin_addr.s_addr = INADDR_ANY;

    if ((net.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        _perror(NULL);

    setsockopt(net.fd, SOL_SOCKET, SO_REUSEADDR, &net.optvalue, sizeof(socklen_t));

    if (bind(net.fd, (struct sockaddr *) &net.addr_s, sizeof(net.addr_s)))
        _perror(NULL);

    listen(net.fd, net.max_conn);

    for (;;)
    {
        net.c_n = sizeof(net.addr_c);
        net._fd = accept(net.fd, (struct sockaddr *) &net.addr_c, &net.c_n);

        if (net._fd < 0)
            _perror(NULL);

        pid = fork();

        if (pid < 0)
            _perror(NULL);
        else if (!pid)
        {
            memset(net.buf, 0, BUFFER_SIZE);

            net.buf_n = recv(net._fd, net.buf, BUFFER_SIZE-1, 0);

            if (net.buf_n < 0)
            {
                errno = 1;
                _perror("Error reading from socket");
            }

            if (net.buf[0] == SERVER_TYPE)
                send(net._fd, "0", 1, 0);
            else
            {
                send(net._fd, "1", 1, 0);
                close(net._fd);
                //continue;
            }

            memset(net.buf, 0, BUFFER_SIZE);

            net.buf_m = BUFFER_SIZE - 1;
            for(;;)
            {
                net.buf_n = recv(net._fd, net.buf, net.buf_m, 0);
                if (net.buf_n == BUFFER_SIZE - 1)
                    break;
                net.buf_m -= net.buf_n;
            }

            m = calloc(strlen(net.buf), sizeof(char));
            strcpy(m, net.buf); 

            // Get key.
            memset(net.buf, 0, BUFFER_SIZE);
            net.buf_m = BUFFER_SIZE - 1;
            for(;;)
            {
                net.buf_n = recv(net._fd, net.buf, net.buf_m, 0);
                if (net.buf_n == BUFFER_SIZE - 1)
                    break;
                net.buf_m -= net.buf_n;
            }

            k = calloc(strlen(net.buf), sizeof(char));
            strcpy(k, net.buf); 

            encode(m, k);

            _debug(m);

            memset(net.buf, 0, BUFFER_SIZE);
            strcpy(net.buf, m);
            send(net._fd, net.buf, BUFFER_SIZE - 1, 0);

            close(net._fd);
            free(m);
            m = 0;
            free(k);
            k = 0;
            exit(EXIT_SUCCESS);
        }
        close(net._fd);
    }
    close(net.fd);
}
