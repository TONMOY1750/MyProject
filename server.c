#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "crypto.h"
#include "utils.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10

typedef struct ClientNode {
    SOCKET socket;
    struct ClientNode *next;
} ClientNode;

ClientNode *client_list_head = NULL;
CRITICAL_SECTION clients_cs;

void add_client(SOCKET sock) {
    EnterCriticalSection(&clients_cs);
    ClientNode *node = (ClientNode*)malloc(sizeof(ClientNode));
    node->socket = sock;
    node->next = client_list_head;
    client_list_head = node;
    LeaveCriticalSection(&clients_cs);
}

void remove_client(SOCKET sock) {
    EnterCriticalSection(&clients_cs);
    ClientNode **current = &client_list_head;
    while (*current) {
        if ((*current)->socket == sock) {
            ClientNode *temp = *current;
            *current = (*current)->next;
            free(temp);
            break;
        }
        current = &((*current)->next);
    }
    LeaveCriticalSection(&clients_cs);
}

void broadcast_data(const char *data, int len, SOCKET sender_sock) {
    EnterCriticalSection(&clients_cs);
    ClientNode *current = client_list_head;
    while (current) {
        if (current->socket != sender_sock) {
            send(current->socket, data, len, 0);
        }
        current = current->next;
    }
    LeaveCriticalSection(&clients_cs);
}

DWORD WINAPI ClientHandler(LPVOID param) {
    SOCKET client_sock = (SOCKET)param;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
        char debug_msg[128];
        sprintf(debug_msg, "Relaying %d ciphertext bytes.", bytes_received);
        log_debug(debug_msg);

        // Zero-Knowledge relay: Server forwards bytes blindly without decryption
        broadcast_data(buffer, bytes_received, client_sock);
    }

    log_info("Client disconnected.");
    remove_client(client_sock);
    closesocket(client_sock);
    return 0;
}

int main(void) {
    WSADATA wsa_data;
    SOCKET server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    InitializeCriticalSection(&clients_cs);

    log_info("Initializing Winsock API...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        log_error("WSAStartup failed.");
        return 1;
    }

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        log_error("Socket creation failed.");
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        log_error("Bind failed.");
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }

    if (listen(server_sock, MAX_CLIENTS) == SOCKET_ERROR) {
        log_error("Listen failed.");
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }

    printf("========================================================\n");
    printf("  SPL1 Zero-Knowledge Relay Server (E2EE)               \n");
    printf("========================================================\n\n");
    log_info("Server is listening on 0.0.0.0:8080");

    while ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len)) != INVALID_SOCKET) {
        log_info("New client socket accepted successfully.");
        add_client(client_sock);
        log_info("Worker thread spawned for new client.");
        CreateThread(NULL, 0, ClientHandler, (LPVOID)client_sock, 0, NULL);
    }

    closesocket(server_sock);
    WSACleanup();
    DeleteCriticalSection(&clients_cs);
    return 0;
}
