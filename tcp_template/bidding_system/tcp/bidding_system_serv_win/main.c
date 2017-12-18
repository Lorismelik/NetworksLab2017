//Server trade

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <time.h> 
#include <stdbool.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define PORT 5001 
#define BUF_SIZE 1000
#define MAIN_MENU "Hello!\nWhat do you want?\n 1.See lot titles\n 2.New lot *only for manager*\n 3.See online users\n 4.Exit\n 5.End *only for manager*"
#define WELCOME "Please type who are you: (login your_login)"
#define LOTS "If you want to make a bet enter new bet, which will be higher than older ('bet lot_name lot_price')\n"
#define NEW_LOT "Please write name and price of the new lot ('lot lot_name lot_price')\n"
#define SUCCESS "The new lot has created!\n"
#define COMMAND_COUNT 10


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
void LotDetail(int lot, int socket, char* out);

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

struct lot {
    char lotName[BUF_SIZE];
    int price;
    char winner[BUF_SIZE];
} *lots;

static char *commands[] = {
    "new",
    "login",
    "0",
    "1",
    "bet",
    "2",
    "lot",
    "3",
    "4",
    "5"
};



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

        h_client = CreateThread(NULL, NULL, &ClientHandler, (void*)(&s1), NULL, &thread_client);
        threads++;
    }
    return 0;
}

DWORD WINAPI *ServerHandler(CONST LPVOID arg) {
    char text[40]; //buffer
    //Getting text from keyboard
    while (1) {

        bzero(text, BUF_SIZE + 1);
        fgets(text, BUF_SIZE, stdin);

        char buf[4];
        strncat(buf, text, 4);
        if (!strcmp(kill_command, buf)) {
            char name[] = "";
            int i = 5;
            while ((text[i] != NULL) && (text[i] != '\n')) {
                name[i] = text[i];
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

    int rc;
    int socket = *(int*)arg;
    
    bool isManager = false;

    //Working 
    while (1) {
        char buf[ BUF_SIZE ]; //Buffer
        char pick[5] = "";
        rc = recv(socket, buf, BUF_SIZE, 0);
        if (rc <= 0)
            SentErrServer("Recv call failed");
        int position = 0;
        while ((buf[position] != ' ') && (buf[position] != '\n') && (buf[position] != NULL) && (position != 5))
        {
            pick[position] = buf[position];
            position++;
        }
        int command = 0;
        while (command <= COMMAND_COUNT)
        {
            if (!strcmp(commands[command], pick))
                break;
            command++;
        }
       
        position++; // to step over 
        switch (command) {
            case 0 : // new
            {
                SendToClient(socket, WELCOME);
                break;
            }
            case 1 : // login
            {
                int i = position;
                char name[BUF_SIZE] = "";
                while ((buf[i] != NULL) && (buf[i] != '\n')) {
                    name[i-position]=buf[i];
                    i++;
                }
                
                //check for manager
                if (!strcmp(name, "manager")) {
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
              
                strcpy(users[threads].login, name);
                SendToClient(socket, MAIN_MENU);
                printf("New user login is: %s\n", users[threads].login);
                printf("\n");
                break;

            }
            case 2: //Menu
            {
                SendToClient(socket, MAIN_MENU);
                break;
            }
            case 3: //See lot titles
            {
                char out[BUF_SIZE] = LOTS;
                for (int i = 0; i <= lotCount; i++) {
                    LotDetail(i, socket, out);
                    strncat(out, "\n", 1);
                }
                SendToClient(socket, out); //send lot names
                break;
                
            }                 
            case 4: // bet lot_name lot_price
            {
                int i = position;
                char name[BUF_SIZE] = "";
                char price[BUF_SIZE] = "";
                while ((buf[i] != NULL) && (buf[i] != ' ') && (buf[i] != '\n'))
                {
                    name[i-position] = buf[i]; 
                    i++;
                }
                i++;
                int j = 0;
                while ((buf[i] != NULL) && (buf[i] != ' ') && (buf[i] != '\n'))
                {
                    price[j] = buf[i]; 
                    i++;
                    j++;
                }
                int lot = FindTitle(name);
                if (lot == -1) {
                    SendToClient(socket, "No lot with this name \n");
                    SendToClient(socket, MAIN_MENU);
                    break;
                }
                SendToClient(socket, SetPrice(lot, price));
                SendToClient(socket, MAIN_MENU);
                break;
            }
            case 5://New lot
            {
                if (isManager == false) {
                    break;
                }

                SendToClient(socket, NEW_LOT);
                break;
            }
            case 6:
            {
                int i = position;
                char name[BUF_SIZE] = "";
                char price[BUF_SIZE] = "";
                while ((buf[i] != NULL) && (buf[i] != ' ') && (buf[i] != '\n'))
                {
                    name[i-position] = buf[i]; 
                    i++;
                }
                i++;
                int j = 0;
                while ((buf[i] != NULL) && (buf[i] != ' ') && (buf[i] != '\n'))
                {
                    price[j] = buf[i]; 
                    i++;
                    j++;
                }
                
                NewLot(name, price, socket);
                SendToClient(socket, SUCCESS);
                SendToClient(socket, MAIN_MENU);
                break; 
            }
            case 7://see online users
            {
                char out[BUF_SIZE] = " ";
                WhoIsOnline(out);
                SendToClient(socket, out);
                SendToClient(socket, MAIN_MENU);
                break;
            }
            case 8://Disconnect client
            {
                printf("%d\n", socket);
                if (isManager == true)
                    manager_count = false;
                SendToClient((int) socket, "#");
                DeleteClient(FindNameBySocket(socket));
                ExitThread(NULL);
                break;
            }
            case 9://See result
            {
                if (isManager == false) {
                    break;
                }
                SendResults();
                EndTrade();
            }
            default://if client type illegal point in main menu
            {
                SendErrorToClient(socket);
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

void LotDetail(int lot, int socket, char* out) {

    strncat(out, lots[lot].lotName, strlen(lots[lot].lotName));
    strncat(out, " ", 1);
    char priceBuf[BUF_SIZE] = "";
    sprintf(priceBuf, "%d", lots[lot].price);
    strncat(out, priceBuf, strlen(priceBuf));
    strncat(out, " ", 1);
    strncat(out, lots[lot].winner, strlen(lots[lot].winner));
    strncat(out, "\n", 1);
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
    strncat(out, "You want to see online users:\n", 30);
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
        SendToClient(socket, "Invalid data \n");
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
    exit(0);

}
