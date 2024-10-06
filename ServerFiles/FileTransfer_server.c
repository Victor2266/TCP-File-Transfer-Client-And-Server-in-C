#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_PACKET_SIZE 100

void send_file(FILE *fp, int sockfd) {
    char buffer[MAX_PACKET_SIZE];
    buffer[0] = 'F';  // Marking first byte to indicate file data
    memset(buffer + 1, 0, MAX_PACKET_SIZE - 1);  // Clear buffer
    size_t n;

    size_t total_bytes_sent = 0;
    // Send file in chunks of MAX_PACKET_SIZE - 1 to account for the marker byte
    while ((n = fread(buffer + 1, 1, MAX_PACKET_SIZE - 1, fp)) > 0) {
        if (send(sockfd, buffer, n + 1, 0) == -1) {
            perror("Error sending file");
            printf("\r-Error-> Sent %zu bytes of data\n", total_bytes_sent);
            exit(1);
        }
        total_bytes_sent += n + 1;
        //printf("buffer header: %c, packet number: %ld\n", buffer[0], total_bytes_sent); //prints the packet headers
        memset(buffer + 1, 0, MAX_PACKET_SIZE - 1);  // Clear buffer
    }
    printf("\r---> Sent %zu bytes of data\n", total_bytes_sent);
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[MAX_PACKET_SIZE] = {0};
    char *error_message = "Error: File not found!";
    
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed!");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for upto 5 connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started\n");
    while (1)
    {
        printf("[Server listening on %s:%d]\n", inet_ntoa(address.sin_addr), PORT);
        // Accept client connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        printf("Client connected\n\n");

        // Receive filename from client
        memset(buffer, 0, MAX_PACKET_SIZE);     //Clear the previous buffer value
        read(new_socket, buffer, MAX_PACKET_SIZE);
        if (strlen(buffer) <= 0) {
            printf("Client disconnected\n\n");
            
        } else {
            printf("Requested file: %s,\n", buffer);

            // Try to open the requested file
            FILE *fp = fopen(buffer, "r");
            if (fp == NULL)
            {
                printf("File not found\n\n");
                // Send error message if file not found
                buffer[0] = 'E'; // Marking first byte to indicate error
                strcpy(buffer + 1, error_message);
                send(new_socket, buffer, strlen(buffer), 0);
            }
            else
            {
                printf("File found\n");
                // Send file data
                buffer[0] = 'F'; // Marking first byte to indicate file data
                send_file(fp, new_socket);
                printf("File Transfer Completed\n\n");
                fclose(fp);
            }
        }

        // Close the socket to signal to the client that the file transfer is complete
        close(new_socket);

    }
    
    close(server_fd);
    
    return 0;
}
