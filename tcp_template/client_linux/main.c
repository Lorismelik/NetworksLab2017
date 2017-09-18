#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

char* readn(int sockfd, char* bytebuffer, int datalen) {
    for (int i = 0; i < datalen; i++) {
        if (read(sockfd, bytebuffer + i, 1) < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }

    }
    return bytebuffer;
}

int main(int argc, char *argv[]) {
    argv[1] = "localhost";
    argv[2] = "5001";
    int sockfd;
    int n;
    uint16_t portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];




    //if (argc < 3) {
    //  fprintf(stderr, "usage %s hostname port\n", argv[0]);
    //  exit(0);
    //}

    portno = (uint16_t) atoi(argv[2]);

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("1");
        perror("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname(argv[1]);

    if (server == NULL) {
        printf("2");
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(server->h_addr, (char *) &serv_addr.sin_addr.s_addr, (size_t) server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("3");
        perror("ERROR connecting");
        exit(1);
    }

    /* Now ask for a message from the user, this message
     * will be read by server
     */

    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    /* Send message to the server */
    n = write(sockfd, buffer, strlen(buffer));

    if (n < 0) {
        printf("4");
        perror("ERROR writing to socket");
        exit(1);
    }

    /* Now read server response */
    bzero(buffer, 256);
    printf("%s\n", readn(sockfd, buffer, 1));
    
    shutdown(sockfd, 2);
    close(sockfd);
    
    return 0;
}

