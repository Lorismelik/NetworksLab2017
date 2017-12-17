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

#define PORT 5001 
#define BUF_SIZE 1000


DWORD WINAPI *ClientHandler(void* socket); //Client handler
DWORD WINAPI *ServerHandler(void* empty); //Server handler
void SendErrorToClient(int socket); // Send error to client
void SentErrServer(char *s); //error handling
void NewLot(char name[], char price[], int socket); //make new lot
int DeleteClient(char name[]); //Delete client
int FindNumberByName(char* name); //Find number of thread by Name
char *FindNameBySocket(int socket); //Find name of client by socket
void WhoIsOnline(char* out); //Make list of online users
char *SetPrice(int lot, char buf[]); //Set price for lot and make status message
void SendToClient(int socket, char* message); //Send message to client
void SendResults(); //Send results to all users
void EndTrade(); //Delete all users
int FindTitle(char title[]);
void SendLotDetail(int lot, int socket);

int threads = -1; //threads counter
int lotCount = -1;
bool manager_count = false; // if manager online

//Command list
char kill_command[] = "kill";
char online_command[] = "online\n";
char shutdown_command[] = "shutdown\n";

struct clients {
    char login[BUF_SIZE];
    int ip;
    int port;
    int s1; // socket for correctly specify the name
    bool manager; // if client is manager
} *users;

DWORD thread_client;
DWORD thread_server;

HANDLE h_client;
HANDLE h_server;

struct lot {
    char lotName[BUF_SIZE];
    int price;
    char winner[BUF_SIZE];
} *lots;

int servSocket;

int main(void) {

    users = (char*) malloc(sizeof (char));
    if (users == NULL) {
        printf("Cant create struct for users");
        EndTrade();
        exit(1);
    }

    lots = (char*) malloc(sizeof (char));
    if (lots == NULL) {
        printf("Cant create struct for lots");
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
    servSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (servSocket < 0)
        SentErrServer("Socket call failed");

    //attach port
    rc = bind(servSocket, (struct sockaddr *) &local, sizeof (local));
    if (rc < 0)
        SentErrServer("Bind call failure");


    h_server = CreateThread(NULL, NULL, &ServerHandler, (void*) NULL, NULL, &thread_server);

    //listening socket
    rc = listen(servSocket, 5);
    if (rc)
        SentErrServer("Listen call failed");

    while (1) {
        //get connection
        s1 = accept(servSocket, (struct sockaddr *) &si_other, &slen);
        if (s1 < 0)
            SentErrServer("Accept call failed");

        //Making new user struct
        users[threads + 1].ip = inet_ntoa(si_other.sin_addr);
        users[threads + 1].port = ntohs(si_other.sin_port);
        users[threads + 1].s1 = s1;
        printf("new socket=%d\n", (int) s1);

        //New thread for client  
        h_client = CreateThread(NULL, NULL, &ClientHandler, (void*)(&s1), NULL, &thread_client);
        threads++;
    }
    return 0;
}

DWORD WINAPI *ServerHandler(CONST LPVOID arg) {
    char text[40]; //buffer
    //Getting text from keyboard
    while (1) {
        
        fgets(text, BUF_SIZE, stdin);
        
        char buf[4];
        strncat(buf, text, 4);
        if (!strcmp(kill_command, buf)) {
            char name[] = "";
            int i = 5;
            while(text[i]!=NULL) {
                name[i]=text[i];
                i++;
            }
            if (!DeleteClient(name))
                printf("All right \n");
            else
                printf("Bad command\n");
        }

        if (!strcmp(online_command, text)) {
            char out[BUF_SIZE] = "";
            WhoIsOnline(out);
            printf(out);
        }


        if (!strcmp(shutdown_command, text)) {
            EndTrade();
            exit(1);
        }
    }
}

void SendToClient(int socket, char* message) {
    int rc;
    rc = send(socket, message, BUF_SIZE, 0);
    if (rc <= 0)
        perror("send call failed");
}

DWORD WINAPI *ClientHandler(CONST LPVOID arg) {
    printf("New user login is:\n");
    int rc;
    int socket = *(int*)arg;
    char buf[ BUF_SIZE ]; //Buffer
    bool isManager = false;
    SendToClient(socket, "Please type who are you: (type *manager* or user *type any name*?:"); //Asking login
    //recieve login
    rc = recv(socket, buf, BUF_SIZE, 0);
    if (rc <= 0)
        SentErrServer("Recv call failed");

    //check for manager
    if (!strcmp(buf, "manager")) {
        isManager = true;
    }

    //delete if manager already exist  
    if (isManager == true) {
        if (manager_count == true) {
            printf("Client was Deleted. Manager already exist\n");
            SendToClient(socket, "#Manager already exist");
            threads--;
            ExitThread(NULL);
        }
    }

    //saving login
        if (isManager) {
            users[threads].manager = true;
            manager_count = true;
        }
        int i = 0;
        while (buf[i] != NULL) {
            users[threads].login[i] = buf[i];
            i++;
        }
        printf("%s\n", users[threads].login);
        memset(buf, 0, BUF_SIZE);
    

    printf("\n");
    //Working 
    char pick = '0';
    while (1) {
        switch (pick) {
            case '0':
            {
                SendToClient(socket, "Hello!\nWhat do you want?\n"
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
                for (int i = 0; i <= lotCount; i++) {
                    strncat(out, lots[i].lotName, strlen(lots[i].lotName));
                    strncat(out, "\n", 1);
                }
                SendToClient(socket, out); //send lot names
                rc = recv((int) socket, buf, BUF_SIZE, 0); //Reading new point of menu
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                //If client want to back
                if (buf[0] == '0') {
                    pick = '0';
                    break;
                }

                int lot = FindTitle(buf);
                if (lot == -1) {
                    pick = '0';
                    break;
                }

                SendLotDetail(lot, socket);

                memset(buf, 0, BUF_SIZE);
                rc = recv(socket, buf, BUF_SIZE, 0); //Reading new message to write
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                if ((buf[0] == '0') && (strlen(buf) == 1)) { //If client want to back
                    pick = '0';
                    break;
                } else {
                    SendToClient(socket, SetPrice(lot, buf));
                    pick = '0';
                    break;
                }
            }
            case '2'://New lot
            {
                memset(buf, 0, BUF_SIZE);
                char name[ BUF_SIZE ];
                char price[ BUF_SIZE ];

                if (isManager == false) {
                    pick = '0';
                    break;
                }

                SendToClient(socket, "Please write name of the lot\n");
                rc = recv((int) socket, name, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed");

                SendToClient(socket, "Please write price of the lot\n");
                rc = recv((int) socket, price, BUF_SIZE, 0);
                if (rc <= 0)
                    SentErrServer("Recv call failed");
                NewLot(name, price, (int) socket);
                pick = '0';
                break;

            }
            case '3'://see online users
            {
                char out[BUF_SIZE] = " ";
                WhoIsOnline(out);
                SendToClient(socket, out);
                pick = '0';
                break;
            }
            case '4'://Disconnect client
            {
                printf("%d\n", socket);
                if (isManager == true)
                    manager_count = false;
                SendToClient((int) socket, "#");
                DeleteClient(FindNameBySocket(socket));
                ExitThread(NULL);
                pick = '0';
                break;
            }
            case '5'://See result
            {
                if (isManager == false) {
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

void SendLotDetail(int lot, int socket) {
    
    char message[BUF_SIZE] = "";   
    strncat(message, lots[lot].lotName, strlen(lots[lot].lotName));
    strncat(message, " ", 1);
    char priceBuf[BUF_SIZE] = "";
    sprintf(priceBuf, "%d", lots[lot].price);
    strncat(message, priceBuf, strlen(priceBuf));
    strncat(message, " ", 1);
    strncat(message, lots[lot].winner, strlen(lots[lot].winner));
    strncat(message, "\n", 1);
    strncat(message, "Enter new bet if you want this lot \n", 36);
    SendToClient(socket, message);
}

int FindTitle(char title[]) {
    for (int i = 0; i <= threads; i++)
        if (!strcmp(lots[i].lotName, title))
            return i;
    return -1;
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
        printf("The client %s was deleted \n", users[number].login);
        if (number != threads) {
            users[number] = users[threads];
            memset(&users[threads], NULL, sizeof (users[threads]));
        }
        threads--;

        return 0;
    }
    return 1;
}

int FindNumberByName(char* name) {
    
    for (int j = 0; j <= threads; j++) {
        if (!strcmp(users[j].login, name))
            return j;
    }
    return -1;
}

void WhoIsOnline(char* out) { 
        strncat(out,"You want to see online users:\n", 30);
        //Show logins
        for (int i = 0; i <= threads; i++) {
            strncat(out, users[i].login, strlen(users[i].login));
            strncat(out, " ", 1);
        }
}

char *SetPrice(int lot, char buf[]) {
    int price = -1;
    price = atoi(buf);
    if (price <= 0) {
        return "New price has wrong format\n";
    }
    if (price <= lots[lot].price) {
        return "New price <= old price!\n";
    } else {
        printf("last_price=%d\n", price);
        lots[lot].price = price;
        return "All right!\n";
    }

}

char *FindNameBySocket(int socket) {
    for (int i = 0; i <= threads; i++) {
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

void NewLot(char name[], char price[], int socket) {
    int newPrice = -1;
    newPrice = atoi(price);
    if (newPrice <= 0 || strlen(name) <= 0) {
        SendErrorToClient(socket);
        return;
    }
    strcpy(lots[lotCount + 1].lotName, name);
    lots[lotCount + 1].price = newPrice;
    strcpy(lots[lotCount + 1].winner, FindNameBySocket(socket));
    lotCount++;
}

void SendResults() {

    for (int i = 0; i <= threads; i++) {
        char result[BUF_SIZE] = "";
        for (int j = 0; j <= lotCount; j++) {
            if (!strcmp(lots[j].winner, users[i].login)) {

                char out[BUF_SIZE] = "You win lot ";
                strcat(out, lots[j].lotName);
                strcat(out, " with price ");
                char priceBuf[BUF_SIZE] = "";
                sprintf(priceBuf, "%d", lots[j].price);
                strcat(out, priceBuf);
                strcat(out, "!\n");
                strcat(result, out);
            } else {

                char out_loser[BUF_SIZE] = "You lose lot ";
                strcat(out_loser, lots[j].lotName);
                strcat(out_loser, " with price ");
                char priceBuf[BUF_SIZE] = "";
                sprintf(priceBuf, "%d", lots[j].price);
                strcat(out_loser, priceBuf);
                strcat(out_loser, "!\n");
                strcat(out_loser, "The winner is ");
                strcat(out_loser, lots[j].winner);
                strcat(out_loser, "!\n");
                strcat(result, out_loser);
            }
        }
        SendToClient((int) users[i].s1, result);
    }
}

void EndTrade() {   
    for (int i = 0; i <= threads; i++) {
        shutdown(users[i].s1, 2);
        close(users[i].s1);
    }
    close(servSocket);
    WSACleanup();

}