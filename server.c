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

// Define the ClientList for client
typedef struct ClientNode
{
    int data;
    struct ClientNode *prev;
    struct ClientNode *link;
    char ip[16];
    char name[31];
} ClientList;

ClientList *newNode(int sock, char *ip)
{
    ClientList *p = (ClientList *)malloc(sizeof(ClientList));
    p->data = sock;
    p->prev = NULL;
    p->link = NULL;
    strncpy(p->ip, ip, 16);
    strncpy(p->name, "NULL", 5);
    return p;
}

// Global variables
int s_sock = 0, c_sock = 0;
ClientList *f, *h;

// Setting exit when detected number of user is empty
void user_exit(int sig)
{
    ClientList *tmp;
    while (f != NULL)
    {
        //Close socket while the user is exit
        printf("\nClose socket: %d\n", f->data);
        close(f->data);
        tmp = f;
        f = f->link;
        free(tmp);
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

// Handle to chat room
void send_to_chatroom(ClientList *p, char tmp_buffer[])
{
    ClientList *tmp = f->link;
    while (tmp != NULL)
    {
        if (p->data != tmp->data)
            send(tmp->data, tmp_buffer, 200, 0);
        tmp = tmp->link;
    }
}

// Main Thread process
void chatroom(void *p_client)
{
    int leave_flag = 0;
    char user[31];
    char recv_buffer[200];
    char send_buffer[200];
    ClientList *p = (ClientList *)p_client;

    // Naming
    if (recv(p->data, user, 20, 0) <= 0 || strlen(user) < 2 || strlen(user) >= 30)
    {
        printf("%s didn't input name.\n", p->ip);
        leave_flag = 1;
    }
    else
    {
        strncpy(p->name, user, 20);
        printf("(%s)%s join the chatroom.\n", p->ip, p->name);
        sprintf(send_buffer, "%s join the chatroom.", p->name);
        send_to_chatroom(p, send_buffer);
    }

    // Conversation
    while (1)
    {
        if (leave_flag)
            break;
        int receive = recv(p->data, recv_buffer, 200, 0);
        if (receive > 0)
        {
            if (strlen(recv_buffer) == 0)
                continue;
            sprintf(send_buffer, "%s：%s", p->name, recv_buffer);
            printf("%s\n", send_buffer);
        }
        // When user exit
        else if (receive == 0 || strcmp(recv_buffer, "exit") == 0)
        {
            printf("(%s)%s leave the chatroom.\n", p->ip, p->name);
            sprintf(send_buffer, "%s leave the chatroom.", p->name);
            leave_flag = 1;
        }
        else
        {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
        send_to_chatroom(p, send_buffer);
    }

    // Remove Node
    close(p->data);
    if (p == h)
    { // remove an edge node
        h = p->prev;
        h->link = NULL;
    }
    else
    { // remove a middle node
        p->prev->link = p->link;
        p->link->prev = p->prev;
    }
    free(p);
}

int main()
{
    struct sockaddr_in server_addr, client_addr;
    int server_address_length = sizeof(server_addr), client_address_length = sizeof(client_addr);

    signal(SIGINT, user_exit);

    // Create socket
    s_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sock == -1)
    {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    // Socket information
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(Server_PortNumber);

    // binding
    if (bind(s_sock, (struct sockaddr *)&server_addr, server_address_length) == -1)
    {
        printf("error binding!\n");
        close(s_sock);
    }

    // listening
    if (listen(s_sock, 20) == -1)
    {
        printf("listen failed!\n");
        close(s_sock);
    }

    // Initial linked list for clients
    f = newNode(s_sock, inet_ntoa(server_addr.sin_addr));
    h = f;

    // Detecting whether has new client join
    while (1)
    {
        // Assign a socket for client
        c_sock = accept(s_sock, (struct sockaddr *)&client_addr, &client_address_length);

        // Print Client IP
        getpeername(c_sock, (struct sockaddr *)&client_addr, &client_address_length);
        printf("Client %s:%d come in.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Append linked list for clients
        ClientList *c = newNode(c_sock, inet_ntoa(client_addr.sin_addr));
        c->prev = h;
        h->link = c;
        h = c;

        // Creat a new thread for socket of client
        pthread_t id;
        if (pthread_create(&id, NULL, (void *)chatroom, (void *)c) != 0)
        {
            perror("Create pthread error!\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}