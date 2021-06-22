#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define Server_PortNumber 9999
#define Server_Address "127.0.0.1"

// Global variables
volatile sig_atomic_t flag = 0;
int sock = 0;
char user[31];

// Trim \n -> NULL for every string
void str_trim_lf(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
}

// Setting enter in terminal
void str_overwrite_stdout()
{
    printf("\r%s", "> ");
    fflush(stdout);
}

// Setting exit flag
void user_exit(int sig)
{
    flag = 1;
}

// Handle receive message
void recv_msg()
{
    char receiveMessage[200];
    while (1)
    {
        //Detecting receive
        int receive = recv(sock, receiveMessage, 200, 0);
        if (receive > 0)
        {
            printf("\r%s\n", receiveMessage);
            str_overwrite_stdout();
        }
        else if (receive == 0)
            break;
        else
        {
        }
    }
}

// Handle the message of that want to send
void send_msg()
{
    char message[200];
    while (1)
    {
        // Preproccess string
        str_overwrite_stdout();
        // Detecting incoming
        while (fgets(message, 200, stdin) != NULL)
        {
            str_trim_lf(message, 200);
            if (strlen(message) == 0)
                str_overwrite_stdout();
            else
                break;
        }
        send(sock, message, 200, 0);
        if (strcmp(message, "exit") == 0)
            break;
    }
    user_exit(2);
}

int main()
{
    struct sockaddr_in server_addr, client_addr;
    int server_address_length = sizeof(server_addr), client_address_length = sizeof(client_addr);

    signal(SIGINT, user_exit);

    // Naming
    printf("Please enter your name: ");
    if (fgets(user, 31, stdin) != NULL)
        str_trim_lf(user, 31);
    if (strlen(user) < 2 || strlen(user) >= 30)
    {
        printf("\nName must be more than one and less than thirty characters.\n");
        exit(EXIT_FAILURE);
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    // Socket information
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(Server_Address);
    server_addr.sin_port = htons(Server_PortNumber);

    // Connect to Server
    if (connect(sock, (struct sockaddr *)&server_addr, server_address_length) < 0)
    {
        printf("Connection to Server error!\n");
        exit(EXIT_FAILURE);
    }

    // Names
    getpeername(sock, (struct sockaddr *)&server_addr, (socklen_t *)&server_address_length);
    printf("Connect to Server: %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    send(sock, user, 31, 0);

    // Create handle send thread
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)send_msg, NULL) != 0)
    {
        printf("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    // Create handle receive thread
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg, NULL) != 0)
    {
        printf("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    // Detecting exit
    while (1)
    {
        if (flag)
        {
            printf("\nBye\n");
            break;
        }
    }

    close(sock);
    return 0;
}