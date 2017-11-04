#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

#include <errno.h>
#include <pthread.h>

void* thread_func(void *arg)
{
    int sockfd = * (int *) arg;
    char buffer[256];
    bzero(buffer, 256);
    int n = read (sockfd, buffer, 255);
    if (n < 0) { perror("ERROR reading from socket");
        exit(1);}
    printf("Here is the message: %s\n", buffer);
    n = write(sockfd, "I have got your message", 23);
    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
        shutdown(sockfd, 2);
        close(sockfd);
    
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd;
    uint16_t portno;
    unsigned int clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    ssize_t n;
    
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5001;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now start listening for the clients, here process will
       * go in sleep mode and will wait for the incoming connection
    */
    while (1) {
           
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    /* Accept actual connection from the client */
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) {
        perror("ERROR on accept");
        exit(1);
    }
    
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, &newsockfd);
    
    }
    return 0;
}


