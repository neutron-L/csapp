#include "sbuf.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define NTHREADS 4
#define SBUFSIZE 16
#define CACHESIZE 10


void doit(int connfd);
void parse_url(char *url, char *hostname, char *port, char *path);
void *thread(void *vargp);


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static sbuf_t sbuf; /* sbuf for request connections fd */
static Cache cache;

int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen;
    pthread_t tid;

    char hostname[MAXLINE], port[MAXLINE];

    if (argc != 2)
    {
        printf("usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    cache_init(&cache, CACHESIZE, MAX_CACHE_SIZE);
    // create worker threads
    for (int i = 0; i < NTHREADS; i++)
        Pthread_create(&tid, NULL, thread,  NULL);

    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, 
        MAXLINE, port, MAXLINE, 0);
        printf("connect to (%s, %s)...\n", hostname, port);
        sbuf_insert(&sbuf, connfd);
    }
    cache_deinit(&cache);

    return 0;
}

void *thread(void *vargp)
{
    Pthread_detach(Pthread_self());

    while (1)
    {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
    }
}


void doit(int connfd)
{
    // define local variables
    rio_t rio_to_client, rio_to_server;
    char buf[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], uri[MAXLINE]; 
    int clientfd;
    int n;

    // init hostname port uri
    memset(hostname, 0, sizeof(hostname));
    memset(port, 0, sizeof(port));
    memset(uri, 0, sizeof(uri));

// init the rio buffer
    Rio_readinitb(&rio_to_client, connfd);
    Rio_readlineb(&rio_to_client, buf, MAXLINE);

// read the request head 
// get the method, url and version
    sscanf(buf, "%s %s %s", method, url, version);

    if (strcasecmp(method, "GET"))
    {
        return;
    }

// find in cache list
    if (read_cache(&cache, url, connfd))
        return;

// parse the url for getting the server host, port and filename
    parse_url(url, hostname, port, uri);

    
    clientfd = Open_clientfd(hostname, port);

    // sent the head
    Rio_readinitb(&rio_to_server, clientfd);
    sprintf(buf, "%s %s HTTP/1.O\r\n", method, uri);
    Rio_writen(clientfd, buf, strlen(buf));

    do
    {
        /* code */
        n = Rio_readlineb(&rio_to_client, buf, MAXLINE);
        Rio_writen(clientfd, buf, n);
    } while (strcmp(buf, "\r\n"));
    
    char cachebuf[MAX_OBJECT_SIZE];
    int bytecount = 0;
    while ((n = Rio_readlineb(&rio_to_server, buf, MAXLINE)) > 0)
    {
        
        bytecount += n;
        if (bytecount < MAX_OBJECT_SIZE)
            strcat(cachebuf, buf);
        
        Rio_writen(connfd, buf, n);
    }

    if (bytecount < MAX_OBJECT_SIZE)
        write_cache(&cache, url, cachebuf);

    Close(connfd);
    Close(clientfd);
}

void parse_url(char *url, char *hostname, char *port, char *path)
{
    char *pre = "http://";
    int prelen;

    if (!strstr(url, pre))
    {
        return;
    }

    prelen = strlen(pre);
    char *start = url + prelen;
    char *end = start + 1;

    while (*end != '/' && *end != ':')
        end++;
    strncpy(hostname, start, end - start);
    if (*end == '/')
        strncpy(port, "80", 2);
    else
    {
        start = end + 1;
        while (*end != '/')
            end++;
        strncpy(port, start, end - start);
    }

    strcpy(path, end);
}
