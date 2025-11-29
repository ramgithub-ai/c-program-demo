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
#include <poll.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define TIMEOUT 1000  // 1 second in milliseconds

// Set socket to non-blocking mode
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    
    // Array of pollfd structures (server + clients)
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;  // Start with just server socket
    
    // Initialize pollfd array
    for (int i = 0; i < MAX_CLIENTS + 1; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;  // Monitor for read events
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
    
    // Add server socket to poll array
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    
    printf("Non-blocking server (using poll) started on port %d\n", PORT);
    printf("Waiting for connections...\n");
    
    while (1) {
        // Wait for activity on any socket
        int activity = poll(fds, nfds, TIMEOUT);
        
        if (activity < 0) {
            perror("Poll error");
            continue;
        }
        
        if (activity == 0) {
            // Timeout - no activity
            continue;
        }
        
        // Check server socket for new connections
        if (fds[0].revents & POLLIN) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    perror("Accept failed");
                }
            } else {
                printf("New connection: socket fd=%d, ip=%s, port=%d\n",
                       new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                
                // Set new socket to non-blocking
                set_nonblocking(new_socket);
                
                // Add new socket to poll array
                if (nfds < MAX_CLIENTS + 1) {
                    fds[nfds].fd = new_socket;
                    fds[nfds].events = POLLIN;
                    nfds++;
                    printf("Added to poll array at index %d (total: %d)\n", nfds - 1, nfds);
                } else {
                    printf("Max clients reached, rejecting connection\n");
                    close(new_socket);
                }
            }
        }
        
        // Check all client sockets for incoming data
        for (int i = 1; i < nfds; i++) {
            if (fds[i].fd < 0) continue;
            
            if (fds[i].revents & POLLIN) {
                memset(buffer, 0, BUFFER_SIZE);
                int valread = read(fds[i].fd, buffer, BUFFER_SIZE);
                
                if (valread == 0) {
                    // Client disconnected
                    getpeername(fds[i].fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Client disconnected: ip=%s, port=%d\n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    
                    close(fds[i].fd);
                    
                    // Remove from poll array by shifting
                    for (int j = i; j < nfds - 1; j++) {
                        fds[j] = fds[j + 1];
                    }
                    fds[nfds - 1].fd = -1;
                    nfds--;
                    i--;  // Adjust index after removal
                    
                } else if (valread > 0) {
                    // Echo back the message
                    buffer[valread] = '\0';
                    printf("Received from client %d: %s", i, buffer);
                    
                    // Send response
                    char response[BUFFER_SIZE];
                    snprintf(response, BUFFER_SIZE, "Server (poll) received: %s", buffer);
                    send(fds[i].fd, response, strlen(response), 0);
                }
            }
            
            // Check for errors
            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                printf("Error on socket %d, closing\n", fds[i].fd);
                close(fds[i].fd);
                
                // Remove from array
                for (int j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                }
                fds[nfds - 1].fd = -1;
                nfds--;
                i--;
            }
        }
    }
    
    return 0;
}
