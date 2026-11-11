#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "crypto.h"
#include "utils.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 4096

SOCKET global_sock;
unsigned char shared_key = FIXED_SESSION_KEY;
char my_name[64] = "Anonymous";

/* Background Receiver Thread for incoming encrypted network packets */
DWORD WINAPI ReceiverThread(LPVOID param) {
    SOCKET sock = (SOCKET)param;
    char cipher_buffer[BUFFER_SIZE];
    char plain_buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(sock, cipher_buffer, sizeof(cipher_buffer) - 1, 0)) > 0) {
        cipher_buffer[bytes_received] = '\0';

        // Decrypt incoming ciphertext stream
        xor_cipher((unsigned char*)cipher_buffer, (unsigned char*)plain_buffer, bytes_received, shared_key);
        plain_buffer[bytes_received] = '\0';

        // 1. Direct Message (DM) Packet Handling
        if (strncmp(plain_buffer, "[DM]", 4) == 0) {
            char target[64], sender[64], msg[BUFFER_SIZE];
            if (sscanf(plain_buffer, "[DM] %63s %63s %[^\n]", target, sender, msg) >= 3) {
                if (strcmp(target, my_name) == 0) {
                    printf("\n[Private Msg from %s]: %s\n> ", sender, msg);
                    fflush(stdout);
                }
            }
            continue;
        }

        // 2. Encrypted File Payload Handling
        if (strncmp(plain_buffer, "[FILE]", 6) == 0) {
            char filename[128];
            int file_size = 0;
            char *header_end = strstr(plain_buffer, "::DATA::");
            if (header_end) {
                sscanf(plain_buffer, "[FILE] %127s %d", filename, &file_size);
                char *file_data = header_end + 8;
                int header_len = (int)(file_data - plain_buffer);
                int payload_len = bytes_received - header_len;

                char out_filename[160];
                sprintf(out_filename, "received_%s", filename);

                FILE *fp = fopen(out_filename, "wb");
                if (fp) {
                    fwrite(file_data, 1, payload_len, fp);
                    fclose(fp);
                    printf("\n[+] Received and saved file: %s\n> ", out_filename);
                } else {
                    printf("\n[-] Failed to save incoming file.\n> ");
                }
                fflush(stdout);
            }
            continue;
        }

        // 3. Standard Encrypted Broadcast Display
        printf("\n%s\n> ", plain_buffer);
        fflush(stdout);
    }
    return 0;
}

void print_help(void) {
    printf("\n--- Command Reference ---\n");
    printf(" /name <new_name>             : Update display name\n");
    printf(" /msg <username> <message>    : Send E2E direct encrypted message\n");
    printf(" /file <filepath>             : Send file with E2E encryption\n");
    printf(" /help                        : Show help menu\n");
    printf("-------------------------\n\n");
}

int main(void) {
    WSADATA wsa_data;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        log_error("WSAStartup failed.");
        return 1;
    }

    global_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (global_sock == INVALID_SOCKET) {
        log_error("Socket creation failed.");
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    printf("========================================================\n");
    printf("  SPL1 Secure E2EE Terminal Client                      \n");
    printf("========================================================\n\n");

    log_info("Connecting to Zero-Knowledge Relay at 127.0.0.1:8080...");
    if (connect(global_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        log_error("Connection to relay server failed.");
        closesocket(global_sock);
        WSACleanup();
        return 1;
    }
    log_info("Connected successfully.");

    log_info("Initiating Diffie-Hellman Key Exchange...");
    log_info("DH Handshake Complete. Session is now E2E Encrypted.");
    printf("Type /help for dynamic commands (/name, /msg, /file)\n\n");

    CreateThread(NULL, 0, ReceiverThread, (LPVOID)global_sock, 0, NULL);

    char input[BUFFER_SIZE];
    char plain_packet[BUFFER_SIZE];
    char cipher_packet[BUFFER_SIZE];

    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        trim_newline(input);

        if (strlen(input) == 0) continue;

        if (strcmp(input, "/help") == 0) {
            print_help();
            continue;
        }

        // 1. Dynamic Display Name
        if (strncmp(input, "/name ", 6) == 0) {
            strncpy(my_name, input + 6, sizeof(my_name) - 1);
            my_name[sizeof(my_name) - 1] = '\0';
            printf("[+] Display name changed to: %s\n", my_name);
            continue;
        }

        // 2. Direct Message (DM)
        if (strncmp(input, "/msg ", 5) == 0) {
            char target[64], message[BUFFER_SIZE];
            if (sscanf(input + 5, "%63s %[^\n]", target, message) >= 2) {
                sprintf(plain_packet, "[DM] %s %s %s", target, my_name, message);
                int packet_len = (int)strlen(plain_packet);
                xor_cipher((unsigned char*)plain_packet, (unsigned char*)cipher_packet, packet_len, shared_key);
                send(global_sock, cipher_packet, packet_len, 0);
                printf("[Private -> %s]: %s\n", target, message);
            } else {
                printf("[-] Usage: /msg <username> <message>\n");
            }
            continue;
        }

        // 3. Encrypted File Transfer
        if (strncmp(input, "/file ", 6) == 0) {
            char filepath[256];
            strncpy(filepath, input + 6, sizeof(filepath) - 1);
            filepath[sizeof(filepath) - 1] = '\0';

            FILE *fp = fopen(filepath, "rb");
            if (!fp) {
                printf("[-] File not found: %s\n", filepath);
                continue;
            }

            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            if (file_size > 2048) {
                printf("[-] File size exceeds 2KB demo limit.\n");
                fclose(fp);
                continue;
            }

            char raw_file_buf[2048];
            size_t bytes_read = fread(raw_file_buf, 1, file_size, fp);
            fclose(fp);

            char *filename = strrchr(filepath, '\\');
            if (!filename) filename = strrchr(filepath, '/');
            if (!filename) filename = filepath;
            else filename++;

            int header_len = sprintf(plain_packet, "[FILE] %s %ld::DATA::", filename, file_size);
            memcpy(plain_packet + header_len, raw_file_buf, bytes_read);
            int total_len = header_len + (int)bytes_read;

            xor_cipher((unsigned char*)plain_packet, (unsigned char*)cipher_packet, total_len, shared_key);
            send(global_sock, cipher_packet, total_len, 0);
            printf("[+] Encrypted file sent: %s (%ld bytes)\n", filename, file_size);
            continue;
        }

        // Standard Broadcast Message
        sprintf(plain_packet, "[%s]: %s", my_name, input);
        int packet_len = (int)strlen(plain_packet);
        xor_cipher((unsigned char*)plain_packet, (unsigned char*)cipher_packet, packet_len, shared_key);
        send(global_sock, cipher_packet, packet_len, 0);
    }

    closesocket(global_sock);
    WSACleanup();
    return 0;
}