//Server trade

#include <sys/types.h>
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <time.h> 
#include <stdbool.h>
#include <string.h>

#define PORT 5001 
#define BUF_SIZE 1000
#define TITLES "titles.txt"

DWORD WINAPI ClientHandler(void* socket); //Client handler
DWORD WINAPI ServerHandler(void* empty); //Server handler
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
//Command list
char kill_command[] = "kill";
char online_command[] = "online";
char shutdown_command[] = "shutdown";

DWORD thread_client;
DWORD thread_server;

HANDLE h_client;
HANDLE h_server;

struct clients {
    char login[BUF_SIZE];
    int ip;
    int port;
    int s1; // socket for correctly specify the name
    bool manager; // if client is manager
} *users;

int s;

int main(void) {

    // SendResults ("New");
    users = (char*) malloc(sizeof (char));
    if (users == NULL) {
        EndTrade();
        exit(1);
    }

    printf("Server trade is working...\n");
    //Initialization
    struct sockaddr_in local, si_other;
    
    WSADATA wsaData;
    ssize_t n;
    n = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (n != 0) {
        printf("WSAStartup failed: %d\n", n);
        return 1;
    }
    
    int s1, rc, slen = sizeof (si_other);
    //fill local
    local.sin_family = AF_INET;
    local.sin_port = htons(PORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    //make socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        SentErrServer("Socket call failed");
    //attach port
    rc = bind(s, (struct sockaddr *) &local, sizeof (local));
    if (rc < 0)
        SentErrServer("Bind call failure");

    //thread for server
    
    h_server = CreateThread(
            NULL, NULL, &ServerHandler, (void*) NULL, NULL, &thread_server);
    //listening socket  
    rc = listen(s, 5);
    if (rc)
        SentErrServer("Listen call failed");
    SeeTitles(); //Get names of themes
    //waiting for clients
    while (1) {
        //get connection
        s1 = accept(s, (struct sockaddr *) &si_other, &slen);

        if (s1 < 0)
            SentErrServer("Accept call failed");
        //Making new user struct

        users[threads + 1].ip = inet_ntoa(si_other.sin_addr);
        users[threads + 1].port = ntohs(si_other.sin_port);
        users[threads + 1].s1 = s1;
        printf("new socket=%d\n", (int) s1);
        //New thread   
        h_client = CreateThread(NULL, NULL, &ClientHandler, (void*)(&s1), NULL, &thread_client);
        threads++;
    }
    return 0;
}

DWORD WINAPI ServerHandler(CONST LPVOID arg) {
    char text[40]; //buffer
    //Getting text from keyboard
    while (1) {
        gets(text);
        if (strstr(kill_command, text) != NULL) {
            char name[] = "";
            strcat(name, &text[5]);
            printf("%s\n", name);
            if (DeleteClient(name) == 0)
                printf("All right. %s was killed\n", name);
            else
                printf("Bad comand. Please try again\n");
        }

        if (strstr(online_command, text) != NULL) {
            WhoIsOnline(-1);
        }

        if (strstr(shutdown_command, text) != NULL) {
            EndTrade();
        }
    }
}

void SendToClient(int socket, char* message) {
    int rc;
    rc = send(socket, message, BUF_SIZE, 0);
    if (rc <= 0)
        perror("send call failed");
}

DWORD WINAPI ClientHandler(CONST LPVOID arg) {
    printf("New user login is:\n");
    int rc;
    int socket = *(int*)arg;
    char buf[ BUF_SIZE ]; //Buffer
    char manager[ BUF_SIZE ] = "manager"; //Buffer
    
   
    recv((int) socket, buf, BUF_SIZE, 0);
    recv((int) socket, buf, BUF_SIZE, 0);
    recv((int) socket, buf, BUF_SIZE, 0);
    recv((int) socket, buf, BUF_SIZE, 0);
    recv((int) socket, buf, BUF_SIZE, 0);
    recv((int) socket, buf, BUF_SIZE, 0);

    memset(buf, 0, BUF_SIZE);
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
            ExitThread(NULL);
        }
    }
    //saving login
    int j = 0;
    for (j; j <= threads; j++) {
        if (users[j].s1 == (int) socket) {
            if (is_manager == true) {
                if (manager_count == false)//save manager
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
                memset(buf, 0, BUF_SIZE);
                char out[BUF_SIZE] = "If you want to open lot, type name of the lot\n";
                strcat(out, themes_names);
                SendToClient((int) socket, out); //send lot names
                rc = recv((int) socket, buf, BUF_SIZE, 0); //Reading new point of menu
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                //If client want to back
                if (buf[0] == '0') {
                    pick = '0';
                    break;
                }

                char filename[] = "";
                strcat(filename, buf);
                strcat(filename, ".txt");

                if (SeeMessages(filename, (int) socket) == 1) {
                    pick = '0';
                    break;
                }
                char buf2[ BUF_SIZE ]; //New buffer only for writing message
                rc = recv((int) socket, buf2, BUF_SIZE, 0); //Reading new message to write
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                if ((buf2[0] == '0') && (strlen(buf2) == 1)) { //If client want to back
                    pick = '0';
                    break;
                } else {
                    WriteMessages(filename, (int) socket, buf2);
                    pick = '0';
                    break;
                }
            }
            case '2'://New lot
            {
                memset(buf, 0, BUF_SIZE);
                char name[ BUF_SIZE ];
                char price[ BUF_SIZE ];

                if (is_manager == false) {
                    pick = '0';
                    break;
                }
                SendToClient((int) socket, "Please write name of the lot\n");
                rc = recv((int) socket, name, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed");

                SendToClient((int) socket, "Please write price of the lot\n");
                rc = recv((int) socket, price, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                NewLot(name, price, (int) socket);
                pick = '0';
                break;

            }
            case '3'://see online users
            {
                WhoIsOnline((int) socket);
                pick = '0';
                break;
            }
            case '4'://Disconnect client
            {
                printf("%d\n", (int) socket);
                if (is_manager == true)
                    manager_count = false;
                SendToClient((int) socket, "#");
                int a = DeleteClient(FindNameBySocket((int) socket));
                ExitThread(NULL);
                pick = '0';
                break;
            }
            case '5'://See result
            {
                if (is_manager == false) {
                    pick = '0';
                    break;
                }
                SendResults();
                EndTrade();

            }
            default://if client type illegal point in main menu
            {
                SendErrorToClient((int) socket);
                pick = '0';
            }
        }
        memset(buf, 0, BUF_SIZE);
    }
    //Disconnect client
    printf("Disconnect client");
    printf("\n");
    threads--;
    ExitThread(NULL);
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
    //printf("%d!\n",number);
    if (number != -1) {
        if (users[number].manager == true)
            manager_count = false;
        //Send signal for client
        SendToClient(users[number].s1, "#");
        //moving cells  
        //printf("%d,%d\n",number, threads);
        if (number != threads) //if it's not the last thread
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
        //Show logins
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
        //Send logins
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
    if (fp == NULL) {
        EndTrade();
        exit(1);
    }
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

    //printf("%s!\n",buf);
    while (buf[i] != NULL) {
        allright = 0;
        j = 0;
        for (j; j <= 9; j++) {
            //printf ("buf=%c number =%c\n", buf[i], numbers[j]);
            if (buf[i] == numbers[j]) {
                //printf("allright=1\n");
                allright = 1;
            }
        }

        if (allright == 0) {
            //printf("ena=0\n");
            enable = 0;
        }
        // printf ("%d\n",i);
        i++;
    }

    if (enable == 1) {
        FILE *fp1;
        fp1 = fopen(filename, "r");
        if (fp1 == NULL) {
            EndTrade();
            exit(1);
        }
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
            if (fp == NULL) {
                EndTrade();
                exit(1);
            }
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
    EndTrade();
    exit(1);
}

void NewLot(char *name, char *price, int socket) {
    //write name
    char file_name[BUF_SIZE] = "";
    strcat(file_name, name);
    strcat(file_name, ".txt");

    FILE *fp1;
    fp1 = fopen(TITLES, "a");
    if (fp1 == NULL) {
        EndTrade();
        exit(1);
    }
    fprintf(fp1, "%s\n", name);
    fclose(fp1);
    SeeTitles(); //refresh
    //write message
    //For local time
    char li[50];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(li, 15, "%d/%m/%Y", timeinfo);

    FILE *fp2;
    fp2 = fopen(file_name, "w");
    if (fp2 == NULL) {
        EndTrade();
        exit(1);
    }
    //printf("%s",filename);
    fprintf(fp2, "--%s<%s> %s\n", price, FindNameBySocket(socket), li);
    fclose(fp2);
}

void SendResults() {
    char str[50];
    int i = 0;
    FILE *fp1;
    fp1 = fopen(TITLES, "r");
    if (fp1 == NULL) {
        EndTrade();
        exit(1);
    }
    int j = 0;
    for (j; j <= threads; j++) {

        char result[BUF_SIZE] = "";
        while (!feof(fp1)) {
            str[i] = fgetc(fp1);
            if (str[i] == -1) break;
            i++;
            if (str[i - 1] == ('\n')) {
                str[i - 1] = NULL;
                if (str[i - 2] == ('\r'))
                str[i - 2] = NULL;

                char* lot_name = str;
                // strcat(lot_name,line1);

                strcat(str, ".txt");


                // printf("filename=%s\n",name);

                size_t len = 0;
                ssize_t read;
                char *line = NULL;

                FILE *fp;
                // perror(j+'0');
                if ((fp = fopen(str, "r")) == NULL) {
                    EndTrade();
                    exit(1);
                }
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
                } else {

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
    WSACleanup();

}
