# Non-Blocking C Network Program

A demonstration of non-blocking TCP server-client communication in C using `select()` for I/O multiplexing.

## Features

- **Non-blocking I/O**: Uses `fcntl()` to set sockets to non-blocking mode
- **I/O Multiplexing**: Uses `select()` to handle multiple clients simultaneously
- **Echo Server**: Server echoes back messages from clients
- **Multi-client Support**: Server can handle up to 10 concurrent clients
- **Graceful Handling**: Proper connection/disconnection handling

## Compilation

```bash
make
```

Or compile manually:
```bash
gcc -Wall -Wextra -O2 -o server server.c
gcc -Wall -Wextra -O2 -o client client.c
```

## Usage

### Start the Server
```bash
./server
```

### Connect with Client
In another terminal:
```bash
./client 127.0.0.1
```

### Test with Multiple Clients
Open multiple terminals and run the client command in each.

## How It Works

### Server (`server.c`)
- Creates a non-blocking TCP socket on port 8080
- Uses `select()` to monitor multiple file descriptors
- Accepts new connections without blocking
- Handles multiple clients simultaneously
- Echoes received messages back to clients

### Client (`client.c`)
- Connects to server in non-blocking mode
- Uses `select()` to monitor both stdin and socket
- Sends user input to server
- Receives and displays server responses
- Type 'quit' to disconnect

## Technical Details

- **Port**: 8080
- **Max Clients**: 10
- **Buffer Size**: 1024 bytes
- **Timeout**: 1 second for select() calls
- **Protocol**: TCP/IP

## Clean Up

```bash
make clean
```
