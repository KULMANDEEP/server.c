#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "common.h"

#define MAX_CLIENT_SUPPORTED    32
#define SERVER_PORT     2000

test_struct_t test_struct;
result_struct_t res_struct;
char data_buffer[1024];

static void intitiaze_monitor_fd_set() {
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
        monitored_fd_set[i] = -1;
}

static void add_to_monitored_fd_set(int skt_fd) {
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitored_fd_set[i] != -1)
            continue;
        monitored_fd_set[i] = skt_fd;
        break;
    }
}

static void remove_from_monitored_fd_set(int skt_fd) {
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitored_fd_set[i] != skt_fd)
            continue;
        monitored_fd_set[i] = -1;
        break;
    }
}

static void re_init_readfds(fd_set *fd_set_ptr) {
    FD_ZERO(fd_set_ptr);
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitored_fd_set[i] != -1) {
            FD_SET(monitored_fd_set[i], fd_set_ptr);
        }
    }
}

static int get_max_fd() {
    int i = 0;
    int max = -1;
    for (; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitored_fd_set[i] > max)
            max = monitored_fd_set[i];
    }
    return max;
}

void setup_tcp_server_communication() {
    int master_sock_tcp_fd = 0, sent_recv_bytes = 0, addr_len = 0, opt = 1;
    int comm_socket_fd = 0;
    fd_set readfds;
    struct sockaddr_in server_addr, client_addr;
    int file_size;
    FILE *received_file;

    intitiaze_monitor_fd_set();

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    addr_len = sizeof(struct sockaddr);

    if (bind(master_sock_tcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        printf("socket bind failed\n");
        return;
    }

    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("listen failed\n");
        return;
    }

    add_to_monitored_fd_set(master_sock_tcp_fd);

    while (1) {
        re_init_readfds(&readfds);
        select(get_max_fd() + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {
            printf("New connection received, accept the connection\n");

            comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (comm_socket_fd < 0) {
                printf("accept error : errno = %d\n", errno);
                exit(0);
            }

            add_to_monitored_fd_set(comm_socket_fd);
            printf("Connection accepted from client: %s:%u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Receive the file size from the client
            if (recv(comm_socket_fd, &file_size, sizeof(int), 0) < 0) {
                perror("File size receive error");
                exit(1);
            }

            // Open a new file for writing the received data
            received_file = fopen("received_file.txt", "wb");
            if (received_file == NULL) {
                perror("Error opening file");
                exit(1);
            }

            // Receive and write the file data
            int remaining_bytes = file_size;
            while (remaining_bytes > 0) {
                int bytes_received = recv(comm_socket_fd, data_buffer, sizeof(data_buffer), 0);
                if (bytes_received < 0) {
                    perror("File receive error");
                    exit(1);
                }
                fwrite(data_buffer, 1, bytes_received, received_file);
                remaining_bytes -= bytes_received;
            }

            fclose(received_file);
            printf("File received successfully\n");

            close(comm_socket_fd);
            remove_from_monitored_fd_set(comm_socket_fd);
        }
    }
}

int main(int argc, char **argv) {
    setup_tcp_server_communication();
    return 0;
}
