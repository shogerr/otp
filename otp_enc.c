#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 70000
#ifdef DECRYPT
#   define CLIENT_TYPE 49
#else
#   define CLIENT_TYPE 48
#endif

struct Client
{
    int fd;
    int port;
    char* buf;
    char buf_[10];
    struct hostent* h;
    struct sockaddr_in addr_s;
};

void _perror(char* s, int i)
{
    if (!errno && i >= 0)
        errno = i;

    perror(s);
    exit(errno);
}

char* parse_file(char* fn, int n)
{
    char* s;
    FILE* f;
    if (!(f = fopen(fn, "r")))
        _perror(NULL, -1);
    s = calloc(n, sizeof(char));
    fgets(s, n, f);
    s[strcspn(s, "\n")] = 0;

    fclose(f);
    return s;
}

int length_check(char* m, char* k)
{
    if (strlen(k) < strlen(m))
        return 1;
    return 0;
}

int range_check(char* s)
{
    int i;
    for (i = 0; i < strlen(s); i++)
    {
        if ((s[i] < 65 || s[i] > 90) && s[i] != 32)
            return 1;
    }
    return 0;
}

void _recv(struct Client* s)
{
    int n;

    // Clear the holding buffer.
    memset(s->buf, 0, BUFFER_SIZE);
    while (strstr(s->buf, "@") == NULL)
    {
        // Clear reading buffer.
        memset(s->buf_, 0, sizeof(s->buf_));
        // Read some bytes.
        n = recv(s->fd, s->buf_, sizeof(s->buf_) - 1, 0);
        if (n < 0) _perror("Socket error", 1);
        // Concat results.
        strcat(s->buf, s->buf_);
        // Break when done.
        if (n == 0)
            break;
    }

    s->buf[strcspn(s->buf, "@")] = '\0';
}

void _send(struct Client* c, char* s)
{
    int n = 0;

    for(;;)
    {
        if ((n += send(c->fd, s + n, strlen(s) - n, 0)) < 0)
            _perror("Socket trouble", 1);
        if (n == strlen(s))
            break;
    }
    send(c->fd, "@", 1, 0);
}

int main (int argc, char** argv)
{
    struct Client c;
    int m;
    int n;
    char* msg;
    char* key;

    // Check number of arguments.
    if (argc < 4)
        _perror(NULL, 1);

    // Get strings of key and message.
    msg = parse_file(argv[1], BUFFER_SIZE);
    key = parse_file(argv[2], BUFFER_SIZE);

    // Check length of key vs message.
    if (length_check(msg, key))
    {
        free(msg);
        msg = 0;
        free(key);
        key = 0;
        _perror("Key is shorter than message", 1);
    }

    if (range_check(msg))
    {
        free(msg);
        msg = 0;
        free(key);
        key = 0;
        _perror("Plaintext contains bad chars", 1);
    }

    //printf("%s", msg);

    bzero(&c.addr_s, sizeof(c.addr_s));
    c.port = atoi(argv[3]);

    c.addr_s.sin_family = AF_INET;
    c.addr_s.sin_port = htons(c.port);
    c.h = gethostbyname("localhost");

    if(!c.h)
        _perror("No such host", 1);

    memcpy((char*)&c.addr_s.sin_addr.s_addr, (char*)c.h->h_addr, c.h->h_length);

    // Initialize socket.
    if ((c.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        _perror("Error opening socket", 1);

    // Connect through socket.
    if (connect(c.fd, (struct sockaddr*)&c.addr_s, sizeof(c.addr_s)) < 0)
        _perror("Error connecting", 1);

    c.buf = malloc(BUFFER_SIZE * sizeof(char));

    memset(c.buf, 0, BUFFER_SIZE);
    c.buf[0] = CLIENT_TYPE;
    m = send(c.fd, c.buf, BUFFER_SIZE-1, 0);

    memset(c.buf, 0, BUFFER_SIZE);
    n = recv(c.fd, c.buf, BUFFER_SIZE-1, 0);

    if (c.buf[0] != 48)
        _perror("Incorrect server", 1);

    _send(&c, msg);

    _send(&c, key);

    _recv(&c);

    printf("%s\n", c.buf);
    close(c.fd);

    free(msg);
    msg = 0;
    free(key);
    key = 0;
    free(c.buf);
    c.buf = 0;
}
