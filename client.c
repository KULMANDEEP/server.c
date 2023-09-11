#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <fcntl.h>  // Added for file handling
#include "common.h"

#define DEST_PORT            2000
#define SERVER_IP_ADDRESS   "127.0.0.1"

#define FILENAME "file_to_send.txt"  // Update with the file you want to send

test_struct_t client_data;
result_struct_t result;

void setup_tcp_communication() {
    int sockfd = 0, sent_recv_bytes = 0;
    int addr_len = 0;
    addr_len = sizeof(struct sockaddr);
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = DEST_PORT;
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    dest.sin_addr = *((struct in_addr *)host->h_addr);
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    connect(sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr));

    // Open the file for reading
    FILE *file = fopen(FILENAME, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Read the file into client_data
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Initialize client_data with file data
    client_data.file_size = file_size;
    fread(client_data.file_data, 1, sizeof(client_data.file_data), file);
    fclose(file);

    // Send the file data to the server
    sent_recv_bytes = sendto(sockfd, &client_data, sizeof(test_struct_t), 0, (struct sockaddr *)&dest, sizeof(struct sockaddr));
    printf("No of bytes sent = %d\n", sent_recv_bytes);

    // Receive the result from the server
    sent_recv_bytes = recvfrom(sockfd, (char *)&result, sizeof(result_struct_t), 0, (struct sockaddr *)&dest, &addr_len);
    printf("No of bytes recvd = %d\n", sent_recv_bytes);
    printf("Result received = %u\n", result.c);

    // Close the socket
    close(sockfd);
}

int main(int argc, char **argv) {
    setup_tcp_communication();
    printf("Application quits\n");
    return 0;
}
