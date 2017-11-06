//Server trade

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <time.h> 
#include <stdbool.h>
#include <string.h>

#define PORT 5001 
#define BUF_SIZE 1000
#define TITLES "titles.txt"

void *ClientHandler(void* socket); //Client handler
void *ServerHandler(void* empty); //Server handler
void SendErrorToClient(int socket); // Send error to client
void SentErrServer(char *s); //error handling
void NewLot(char *name, char *price, int socket); //make new lot
int DeleteClient(char name[]); //Delete client
int FindNumberByName(char name[]); //Find number of thread by Name
char *FindNameBySocket(int socket); //Find name of client by socket
void WhoIsOnline(int socket); //Know who is online
void SeeTitles(); //To see titles of themes
int SeeMessages(char filename[], int socket); //See masseges in theme
void WriteMessages(char filename[], int socket, char buf[]); //Write message to theme
void SendToClient(int socket, char* message); //Send something to client
void SendResults();
void EndTrade();
int threads = -1; //threads counter
bool manager_count = false; // if manager online
char themes_names[BUF_SIZE]; //lot names
//Comand list
char kill_comand[] = "kill";
char online_comand[] = "online";

struct clients {
    char login[BUF_SIZE];
    int ip;
    int port;
    int s1;
    bool manager; // if client is manager
} *users;

int s;

int main(void) {

    // SendResults ("New");


    users = (char*) malloc(sizeof (char));
    if (users == NULL) {
        perror("Memory error");
        exit(1);
    }

    printf("Server trade is working...\n");
    //Initialization
    struct sockaddr_in local, si_other;
    int s1, rc, slen = sizeof (si_other);
    //fill local
    local.sin_family = AF_INET;
    local.sin_port = htons(PORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        SentErrServer("Socket call failed");

    rc = bind(s, (struct sockaddr *) &local, sizeof (local));
    if (rc < 0)
        SentErrServer("Bind call failure");

    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    // Server
    pthread_t server_thread;
    rc = pthread_create(&server_thread, &threadAttr, ServerHandler, (void*) NULL);

    rc = listen(s, 5);
    if (rc)
        SentErrServer("Listen call failed");
    while (1) {
        //get connection
        s1 = accept(s, (struct sockaddr *) &si_other, &slen);

        if (s1 < 0)
            SentErrServer("Accept call failed");


        users[threads + 1].ip = inet_ntoa(si_other.sin_addr);
        users[threads + 1].port = ntohs(si_other.sin_port);
        users[threads + 1].s1 = s1;
        printf("new socket=%d\n", (int) s1);

        pthread_t client_thread;

        rc = pthread_create(&client_thread, &threadAttr, ClientHandler, (void*) s1);

        if (rc != 0)
            SentErrServer("Creating thread false");
        threads++;
    }
    return 0;
}

void *ServerHandler(void* empty) {
    char text[40]; 
    while (1) {
        gets(text);
        int i = 0;
        int right_comand = 1;

        if (strstr(kill_comand, text) != NULL) {
            char name[] = "";
            strcat(name, &text[5]);
            printf("%s\n", name);
            if (DeleteClient(name) == 0)
                printf("All right. %s was killed\n", name);
            else
                printf("Bad command. Please try again\n");
        }

        if (strstr(online_comand, text) != NULL) {
            WhoIsOnline(-1);
        }
    }
}

void SendToClient(int socket, char* message) {
    int rc;
    rc = send(socket, message, BUF_SIZE, 0);
    if (rc <= 0)
        perror("send call failed");
}

void *ClientHandler(void* socket) {
    printf("New user login is:\n");
    int rc;
    char buf[ BUF_SIZE ]; 
    char manager[ BUF_SIZE ] = "manager"; 
    bool is_manager;
    SendToClient((int) socket, "Please type who are you: manager(type *manager* or user *type any name*?:"); //Asking login
    //recive login
    rc = recv((int) socket, buf, BUF_SIZE, 0);
    if (rc <= 0)
        SentErrServer("Recv call failed");
    //is name = manager?
    int i = 0;
    is_manager = true;
    while (buf[i] != NULL) {
        if (buf[i] != manager[i]) {
            is_manager = false;
            break;
        }
        i++;
    }
    //delete if manager already exist  
    if (is_manager == true) {
        if (manager_count == true) {
            printf("Client was Deleted. Manager already exist\n");
            SendToClient((int) socket, "#Manager already exist");
            threads--;
            pthread_exit(NULL);
        }
    }
    int j = 0;
    for (j; j <= threads; j++) {
        if (users[j].s1 == (int) socket) {
            if (is_manager == true) {
                if (manager_count == false)
                {
                    users[j].manager == true;
                    manager_count = true;
                }
            }
            int i = 0;
            while (buf[i] != NULL) {
                users[j].login[i] = buf[i];
                i++;
            }
            printf("%s\n", users[threads].login);
        }
    }
    printf("\n");
    //Working 
    char pick = '0';
    while (1) {
        switch (pick) {
            case '0':
            {
zero_menu_send:
                SendToClient((int) socket, "Hello!\nWhat do you want?\n"
                        "1.See lot titles\n"
                        "2.New lot *only for manager*\n"
                        "3.See online users\n4.Exit\n5.End *only for manager*");
                rc = recv((int) socket, buf, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed when pick was 0");
                pick = buf[0];
                break;
            }
            case '1'://See lot titles
            {
                char out[BUF_SIZE] = "If you want to open lot, type name of the lot\n";
                char name[BUF_SIZE] = {NULL};
                strcat(out, themes_names);
                SendToClient((int) socket, out); 
                rc = recv((int) socket, buf, BUF_SIZE, 0); 
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                //If client want to back
                if (buf[0] == '0') {
                    pick = '0';
                    break;
                }


                {
                    char filename[] = "";
                    strcat(filename, buf);
                    strcat(filename, ".txt");

                    if (SeeMessages(filename, (int) socket) == 1) {
                        goto zero_menu_send;
                    }
                    char buf2[ BUF_SIZE ]; 
                    rc = recv((int) socket, buf2, BUF_SIZE, 0); 
                    if (rc <= 0)
                        SentErrServer("Recv call failed");
                    if (buf2[0] == '0') 
                        goto zero_menu_send;

                    else {
                        WriteMessages(filename, (int) socket, buf2);
                        goto zero_menu_send;
                    }
                }
            }
            case '2'://New lot
            {
                char name[ BUF_SIZE ];
                char price[ BUF_SIZE ];

                if (is_manager == false)
                    goto zero_menu_send;

                SendToClient((int) socket, "Please write name of the lot\n");
                rc = recv((int) socket, name, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                printf("%s name\n", name);

                SendToClient((int) socket, "Please write price of the lot\n");
                rc = recv((int) socket, price, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                printf("%s price\n", price);
                printf("%s %s name\n", name, price);
                NewLot(name, price, (int) socket);
                goto zero_menu_send;
                break;

            }
            case '3'://see online users
            {
                WhoIsOnline((int) socket);
                //Get new point of menu
                rc = recv((int) socket, buf, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                pick = buf[0];
                break;
            }
            case '4'://Disconnect client
            {
                printf("%d\n", (int) socket);
                if (is_manager == true)
                    manager_count = false;
                SendToClient((int) socket, "#");
                int a = DeleteClient(FindNameBySocket((int) socket));
                pthread_exit(NULL);
                break;
            }
            case '5'://See result
            {
                if (is_manager == false)
                    goto zero_menu_send;
                SendResults();
                EndTrade();

            }
            default:
            {
                printf("lol");
                SendErrorToClient((int) socket);
                pick = '0';
                buf[0] == '0';
            }
        }
    }
   
    printf("Disconnect client");
    printf("\n");
    threads--;
    pthread_exit(NULL);
}

void SendErrorToClient(int socket) {
    int rc = 0;
    rc = send((int) socket, "^", 1000, 0);
    if (rc <= 0)
        perror("send call failed");
}

int DeleteClient(char name[]) {
    printf("DeleteClient running...\n");
    int number;
    number = FindNumberByName(name);
    if (number != -1) {
        if (users[number].manager == true)
            manager_count = false;

        SendToClient(users[number].s1, "#");

        if (number != threads) 
        {
            users[number] = users[threads];
            memset(&users[threads], NULL, sizeof (users[threads]));
        }
        threads--;

        return 0;
    }
    return 1;
}

int FindNumberByName(char name[]) {
    int j = 0;
    int i = 0;
    int rightflag;
    for (j; j <= threads; j++) {
        rightflag = 1;
        i = 0;
        while (name[i] != NULL) {
            if (users[j].login[i] != name[i]) {
                rightflag = 0;
            }
            i++;
        }
        if (rightflag == 1) {
            return j;
        }
    }
    return -1;
}

void WhoIsOnline(int socket) {
    if (socket == -1) {
        char out[BUF_SIZE] = "You want to see online users:\n";
        int i = 0;
        for (i; i <= threads; i++) {
            strcat(out, users[i].login);
            strcat(out, " ");
        }
        printf("%s\n", out);
    } else {
        int rc;
        char out[BUF_SIZE] = "You want to see online users:\n";
        int i = 0;
        //Show logins
        for (i; i <= threads; i++) {
            strcat(out, users[i].login);
            strcat(out, " ");
        }
        SendToClient(socket, out);
    }
}

void SeeTitles() {
    *themes_names = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE *fp;
    fp = fopen(TITLES, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1) {
        strcat(themes_names, line);
    }
    fclose(fp);
}

int SeeMessages(char filename[], int socket) {
    size_t len = 0;
    ssize_t read;
    char *line = NULL;
    char out[BUF_SIZE] = "";
    int rc;
    FILE *fp;
    //printf("%s filename\n",filename);
    if ((fp = fopen(filename, "r")) == NULL)
        return 1;
    while ((read = getline(&line, &len, fp)) != -1) {
        strcat(out, line);
    }
    fclose(fp);
    strcat(out, "You can write new price or 0 to back");
    SendToClient(socket, out);
    return 0;
}

void WriteMessages(char filename[], int socket, char buf[]) {
    char numbers[9];
    numbers[0] = '0';
    numbers[1] = '1';
    numbers[2] = '2';
    numbers[3] = '3';
    numbers[4] = '4';
    numbers[5] = '5';
    numbers[6] = '6';
    numbers[7] = '7';
    numbers[8] = '8';
    numbers[9] = '9';

    int allright = 0; // 1 if all is good
    int enable = 1; // 1 if it is numbers

    int i = 0;
    int j = 0;


    while (buf[i] != NULL) {
        allright = 0;
        j = 0;
        for (j; j <= 9; j++) {
 
            if (buf[i] == numbers[j]) {
                allright = 1;
            }
        }

        if (allright == 0) {
            enable = 0;
        }

        i++;
    }

    if (enable == 1) {
        FILE *fp1;
        fp1 = fopen(filename, "r");
        if (fp1 == NULL)
            exit(1);
        size_t len = 0;
        ssize_t read;
        char *line = NULL;
        while ((read = getline(&line, &len, fp1)) != -1) {
        }

        char last_price[100] = {};

        int i = 0;
        while (line[i] != '<') {
            i++;
        }

        memcpy(last_price, line + 2, (i - 2) * sizeof (char));

        printf("%s\n", last_price);

        int last_price_int = atoi(last_price);
        int new_price_int = atoi(buf);

        if (new_price_int <= last_price_int) {
            fclose(fp1);
            SendToClient(socket, "New price <= old price!\n");
        } else {
            fclose(fp1);
            printf("last_price=%s\n", last_price);

            //For local time
            char li[50];
            time_t rawtime;
            struct tm * timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(li, 15, "%d/%m/%Y", timeinfo);

            FILE *fp;
            fp = fopen(filename, "a");
            if (fp == NULL)
                exit(1);
            //printf("%s",filename);
            fprintf(fp, "--%s<%s> %s\n", buf, FindNameBySocket(socket), li);
            fclose(fp);
            SendToClient(socket, "All right!\n");
        }
    } else {
        SendToClient(socket, "Wrong format of price!");
    }
}

char *FindNameBySocket(int socket) {
    int i = 0;
    for (i; i <= threads; i++) {
        if (users[i].s1 == socket)
            return users[i].login;
    }
    return "error";
}

void SentErrServer(char *s) //error handling
{
    perror(s);
    exit(1);
}

void NewLot(char *name, char *price, int socket) {
    //write name
    char file_name[BUF_SIZE] = "";
    strcat(file_name, name);
    strcat(file_name, ".txt");

    FILE *fp1;
    fp1 = fopen(TITLES, "a");
    if (fp1 == NULL)
        exit(1);
    fprintf(fp1, "%s\n", name);
    fclose(fp1);
    SeeTitles(); //refresh

    char li[50];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(li, 15, "%d/%m/%Y", timeinfo);

    FILE *fp2;
    fp2 = fopen(file_name, "w");
    if (fp2 == NULL)
        exit(1);

    fprintf(fp2, "--%s<%s> %s\n", price, FindNameBySocket(socket), li);
    fclose(fp2);
    SendToClient(socket, "All right!\n Hello!\nWhat are you want?\n"
            "1.See lot titles\n"
            "2.New lot *only for manager*\n"
            "3.See online users\n4.Exit\n");

}

void SendResults() {
    char str[50];
    int i = 0;
    FILE *fp1;
    fp1 = fopen(TITLES, "r");
    if (fp1 == NULL)
        exit(EXIT_FAILURE);
    int j = 0;
    for (j; j <= threads; j++) {

        char result[BUF_SIZE] = "";
        while (!feof(fp1)) {
            str[i] = fgetc(fp1);
            if (str[i] == -1) break;
            i++;
            if (str[i - 1] == ('\n')) {
                str[i - 1] = NULL;
                str[i - 2] = NULL;

                char* lot_name = str;

                strcat(str, ".txt");

                size_t len = 0;
                ssize_t read;
                char *line = NULL;

                FILE *fp;
                // perror(j+'0');
                if ((fp = fopen(str, "r")) == NULL)
                    return 1;
                while ((read = getline(&line, &len, fp)) != -1) {
                }

                i = 0;

                int start = 0;
                int stop = 0;

                while (line[i] != NULL) {
                    if (line[i] == '<')
                        start = i;
                    if (line[i] == '>')
                        stop = i;
                    i++;
                }

                char last_price[100] = {};

                memcpy(last_price, line + 2, (start - 2) * sizeof (char));



                char winner[100] = {};
                memcpy(winner, line + start + 1, (stop - start - 1) * sizeof (char));
                if (strstr(winner, users[j].login) != NULL) {
                    char out[BUF_SIZE] = "You win lot ";
                    strcat(out, lot_name);
                    strcat(out, " with price ");
                    strcat(out, last_price);
                    strcat(out, "!\n");
                    strcat(result, out);
                }
                else {

                    char out_looser[BUF_SIZE] = "You lose lot ";
                    strcat(out_looser, lot_name);
                    strcat(out_looser, " with price ");
                    strcat(out_looser, last_price);
                    strcat(out_looser, "!\n");
                    strcat(out_looser, "The winner is ");
                    strcat(out_looser, winner);
                    strcat(out_looser, "!\n");
                    strcat(result, out_looser);
                }


                fclose(fp);
                memset(str, 0, 50);
                i = 0;
            }
        }
        rewind(fp1);
        SendToClient((int) users[j].s1, result);
        memset(result, 0, BUF_SIZE);
    }
}

void EndTrade() {
    int i = 0;
    for (i; i <= threads; i++) {
        shutdown(users[i].s1, 2);
        close(users[i].s1);
    }

}