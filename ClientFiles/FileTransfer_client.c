#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_TCP_PORT 8080	/* well-known port */
#define MAX_PACKET_SIZE 100

int main(int argc, char *argv[]) {
    int sock = 0, valread;
    struct	hostent		*hp;
    struct sockaddr_in serv_addr;
    char buffer[MAX_PACKET_SIZE] = {0};

    int server_port = SERVER_TCP_PORT;
    char *server_ip;

    char filename[50];

    switch (argc){
    // Check if IP and port are provided as arguments
    case 2:
        server_ip = argv[1];
        server_port = SERVER_TCP_PORT;
        break;
    case 3:
        server_ip = argv[1];
        server_port = atoi(argv[2]);
        break;
    default:
        printf("Usage: %s <IP Address> <Port>\n", argv[0]);
        printf("Or: %s <IP Address>\n", argv[0]);
        return -1;
    }

    // Set server address
    bzero((char *)&serv_addr, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    if (hp = gethostbyname(server_ip)) {
        bcopy(hp->h_addr, (char *)&serv_addr.sin_addr, hp->h_length);
    } else if (inet_aton(server_ip, (struct in_addr *) &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Can't get server's address\n");
        printf("Invalid address / Address not supported\n");
		exit(1);
    }

    while (1)
    {
        // Create client socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Socket creation error\n");
            return -1;
        }

        // Connect to server
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("Connection Failed\n");
            return -1;
        }

        // Get filename from user
        printf("Enter the filename to download: ");
        memset(filename, 0, 50);
        if (scanf("%s", filename) != 1 || strlen(filename) == 0)
        {
            printf("No filename provided, closing connection...\n");
            close(sock); //When the server reads it will return ECONNRESET
            return 0; // Exit if no filename is provided
        }


        // Send filename to server
        if (send(sock, filename, strlen(filename), 0) <= 0)
        {
            printf("Local error sending filename to server\n");
            return -1;
        }

        // Open file to save received data
        FILE *fp = fopen(filename, "w");
        if (fp == NULL)
        {
            printf("Error opening file to write\n");
            return -1;
        }

        // Receive file data or error message
        int n = 0;
        int total_bytes = 0;
        while ((n = recv(sock, buffer, MAX_PACKET_SIZE, MSG_WAITALL)) > 0)
        {
            total_bytes += n;
            if (buffer[0] == 'E')
            {
                printf("\nPacket Header: [%c] Packet Code: %d\n", buffer[0], total_bytes); // Print error message
                printf("Error: [%s]\n", buffer + 1); // Print error message
                break;
            }
            else if (buffer[0] == 'F')
            {
                // Write file data (excluding the first byte, which is used as a marker)
                fwrite(buffer + 1, 1, n - 1, fp);  // Write n-1 bytes (since buffer[0] is a marker)

                //printf("Writing String: [%s]\n", buffer + 1);
                //printf("\nReceived packet header: %c, Packet Code: %d", buffer[0], total_bytes); // Print error message
                printf("\r---> Received %d bytes of data.", total_bytes);
            }
            memset(buffer, 0, MAX_PACKET_SIZE);     // Clear buffer
        }

        // Check if file was received successfully
        //printf("\nBuffer Code: %c\n", buffer[0]);
        if(buffer[0] == 'E'){
            printf("\nError receiving file\n\n");
            remove(filename);
            break;
        }else if(total_bytes == 0){
            printf("\nError in server connection, nothing was transfered\n\n");
            remove(filename);
            break;
        }else{ 
            printf("\nFile received successfully\n\n");
        }

        // Close file
        fclose(fp);
        
    }
   
    // Close socket
    printf("closing socket\n\n");
    close(sock);

    return 0;
}
