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
#define BUFFER_SIZE 1024
#define TIMEOUT 1000  // 1 second in milliseconds

// Set socket to non-blocking mode
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    struct pollfd fds[2];  // stdin and socket
    
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        printf("Example: %s 127.0.0.1\n", argv[0]);
        return -1;
    }
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }
    
    // Set socket to non-blocking
    if (set_nonblocking(sock) < 0) {
        perror("Failed to set non-blocking");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported\n");
        return -1;
    }
    
    // Connect to server (non-blocking)
    printf("Connecting to server %s:%d...\n", argv[1], PORT);
    int result = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    if (result < 0) {
        if (errno == EINPROGRESS) {
            // Connection in progress - use poll to wait
            struct pollfd connect_fd;
            connect_fd.fd = sock;
            connect_fd.events = POLLOUT;
            
            if (poll(&connect_fd, 1, 5000) > 0) {
                int error;
                socklen_t len = sizeof(error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
                
                if (error != 0) {
                    printf("Connection failed: %s\n", strerror(error));
                    return -1;
                }
                printf("Connected to server!\n");
            } else {
                printf("Connection timeout\n");
                return -1;
            }
        } else {
            printf("Connection failed\n");
            return -1;
        }
    } else {
        printf("Connected to server!\n");
    }
    
    // Setup poll array
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    
    fds[1].fd = sock;
    fds[1].events = POLLIN;
    
    printf("\nNon-blocking client (using poll) ready. Type messages to send (or 'quit' to exit):\n");
    
    while (1) {
        int activity = poll(fds, 2, TIMEOUT);
        
        if (activity < 0) {
            printf("Poll error\n");
            break;
        }
        
        // Check if user typed something
        if (fds[0].revents & POLLIN) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                if (strncmp(buffer, "quit", 4) == 0) {
                    printf("Closing connection...\n");
                    break;
                }
                
                // Send message to server
                int sent = send(sock, buffer, strlen(buffer), 0);
                if (sent < 0) {
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        printf("Send failed\n");
                        break;
                    }
                } else {
                    printf("Message sent (%d bytes)\n", sent);
                }
            }
        }
        
        // Check if server sent something
        if (fds[1].revents & POLLIN) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(sock, buffer, BUFFER_SIZE);
            
            if (valread == 0) {
                printf("Server disconnected\n");
                break;
            } else if (valread > 0) {
                printf("Server response: %s\n", buffer);
            } else {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    printf("Read error\n");
                    break;
                }
            }
        }
        
        // Check for errors
        if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            printf("Connection error or closed\n");
            break;
        }
    }
    
    close(sock);
    printf("Connection closed\n");
    return 0;
}
