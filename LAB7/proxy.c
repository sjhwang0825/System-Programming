#include <stdio.h>
#include "csapp.h"
#include <assert.h>

/* You won't lose points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void request(int connfd);
void parse(char *uri, char *hostname, char *port, char *path);
void getheader(rio_t client, char *hostname, char *path, char *header);
void *request_thread(void *vargp);


//refer to the book and Lecture PPT
int main(int argc, char **argv)
{
    int listenfd, *connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    char* port=argv[1];

    listenfd=Open_listenfd(port);
    pthread_t tid;
    while(1){
        connfd=malloc(4);
        clientlen=sizeof(struct sockaddr_in);
        *connfd=Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,client_port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", client_hostname, client_port);
        pthread_create(&tid, NULL, request_thread, connfd);
    }
    return 0;
}

/*refer to Lecture 17 PPT*/
void *request_thread(void *vargp){
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    request(connfd);
    Close(connfd);
    return NULL;
}


void request(int connfd){
    char buf[MAXBUF], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    rio_t client;
    Rio_readinitb(&client, connfd);
    Rio_readlineb(&client, buf, MAXBUF);

    /*find method, URI, and version*/
    sscanf(buf, "%s %s %s",method, uri, version);

    if(0!=strcmp("GET",method)){
        fprintf(stderr,"Only GET method is implemented\n");
        return;
    }

    /*hostname and port num is necessary to use open_clientfd function*/
    /* the hostname, www.cmu.edu; and the path or query and everything following it*/
    //why malloc makes error?

    char hostname[MAXLINE], port[16], path[MAXLINE]; //port is 16 bits
    parse(uri, hostname, port, path);
    
    //printf("\n%s\n%s\n%s\n%s\n",uri, hostname, path, port);

    char header[MAXLINE];
    getheader(client, hostname, path, header);
    printf("%s",header);

    //proxy establish a connection with a server
    int connect;
    if(-1==(connect=open_clientfd(hostname, port))){
        fprintf(stderr,"ERROR: Unable to make Connection\n");
        assert(0);
    }

    //proxy send header from proxy to server
    rio_t p2s;
    Rio_readinitb(&p2s, connect);
    Rio_writen(connect, header, strlen(header));

/*  get HTTP response
 *  example of HTTp response
 *  HTTP/1.1 301 Moved Permanently
 *  Date:
 *  Server:
*/
    /*Add to match the output result with tiny.*/
    printf("Response headers:\n");
    size_t size_res;
    for(int i=0 ; 0!=(size_res=Rio_readlineb(&p2s, buf, MAXLINE)) ; i++){
        Rio_writen(connfd, buf, size_res);
        if(i<5) printf("%s",buf); //Add to match the output result with tiny.
    }

    Close(connect);    
}

void parse(char *uri, char *hostname, char *port, char *path){
    /*example; walkeshart.ics.cs.cmu.edu:15213/cgi-bin/adder?15213&18213 */
    /*example; https://www.google.com*/
    //walk1 is for finding hostname, walk2 is for port, walk3 is for path
    char *walk1, *walk2, *walk3;
    strcpy(port,"80");//default port

    walk1=strstr(uri, "//");
    if(NULL!=walk1) walk1+=2;
    else walk1=uri;

    //finding port, that is finding ':'
    walk2=strchr(walk1, ':');
    if(NULL!=walk2){
        *walk2='\0';
        strcpy(hostname, walk1);
        *walk2=':';
        //finding path info, that is finding '/'
        walk3=strchr(walk2,'/');
        if(NULL!=walk3){
            *walk3='\0';
            strcpy(port, walk2+1);
            *walk3='/';
            strcpy(path, walk3);
        }
        else{
            strcpy(port, walk2+1);
        }
    }
    else{
        walk3=strchr(walk1, '/');
        if(NULL!=walk3){
            *walk3='\0';
            strcpy(hostname, walk1);
            *walk3='/';
            strcpy(path, walk3);
        }
        else{
            strcpy(hostname, walk1);
        }
    }
}

//Refer to the Lecture16 PPT code related to sprintf.
void getheader(rio_t client, char *hostname, char *path, char *header){
    char cache[MAXLINE];
    strcpy(cache, header);

    sprintf(cache,"GET %s HTTP/1.0\r\n",path);
    strcpy(header, cache);

    sprintf(cache,"%sHost: %s\r\n",header,hostname);
    strcpy(header, cache);

    sprintf(cache,"%sUser-Agent: %s",header,user_agent_hdr);
    strcpy(header, cache);

    sprintf(cache,"%sConnection: close\r\n",header);
    strcpy(header, cache);

    sprintf(cache,"%sProxy-Connection: close\r\n",header);
    strcpy(header, cache);

    //find additional header
    char buf[MAXLINE];
    while(Rio_readlineb(&client, buf, MAXLINE)>0){
        if(0==strcmp(buf, "\r\n")) break;
        
        if(0==strncmp(buf, "Host", strlen("Host"))) continue;
        else if(0==strncmp(buf, "User-Agent", strlen("User-Agent"))) continue;
        else if(0==strncmp(buf, "Connection", strlen("Connection"))) continue;
        else if(0==strncmp(buf, "Proxy-Connection", strlen("Proxy-Connection"))) continue;
        else {
            sprintf(cache,"%s%s",header,buf);
            strcpy(header, cache);
        }
    }

    //end of HTTP request
    sprintf(cache,"%s\r\n",header);
    strcpy(header, cache);
}
