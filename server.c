#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Set socket to non-blocking mode
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS];
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_sd, sd, activity, i, valread;
    
    // Initialize all client sockets to 0
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }
    
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket to non-blocking
    if (set_nonblocking(server_fd) < 0) {
        perror("Failed to set non-blocking");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Bind socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Non-blocking server started on port %d\n", PORT);
    printf("Waiting for connections...\n");
    
    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);
        
        // Add server socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        
        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            
            if (sd > 0)
                FD_SET(sd, &readfds);
            
            if (sd > max_sd)
                max_sd = sd;
        }
        
        // Wait for activity on any socket (non-blocking with timeout)
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        
        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error\n");
        }
        
        // If something happened on the server socket, it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    perror("Accept failed");
                }
            } else {
                printf("New connection: socket fd=%d, ip=%s, port=%d\n",
                       new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                
                // Set new socket to non-blocking
                set_nonblocking(new_socket);
                
                // Add new socket to array of sockets
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_socket;
                        printf("Added to client list at index %d\n", i);
                        break;
                    }
                }
            }
        }
        
        // Check all client sockets for incoming data
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                valread = read(sd, buffer, BUFFER_SIZE);
                
                if (valread == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Client disconnected: ip=%s, port=%d\n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    
                    close(sd);
                    client_sockets[i] = 0;
                } else if (valread > 0) {
                    // Echo back the message
                    buffer[valread] = '\0';
                    printf("Received from client %d: %s", i, buffer);
                    
                    // Send response
                    char response[BUFFER_SIZE];
                    snprintf(response, BUFFER_SIZE, "Server received: %s", buffer);
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }
    
    return 0;
}
