# Non-Blocking C Network Program - poll() vs select()

A comprehensive demonstration of non-blocking TCP server-client communication in C, comparing `select()` and `poll()` for I/O multiplexing.

## Features

- **Non-blocking I/O**: Uses `fcntl()` to set sockets to non-blocking mode
- **Two I/O Multiplexing Methods**: Both `select()` and `poll()` implementations
- **Echo Server**: Server echoes back messages from clients
- **Multi-client Support**: Server can handle up to 10 concurrent clients
- **Graceful Handling**: Proper connection/disconnection handling
- **Side-by-side Comparison**: See the differences in action

## Files

### select() Implementation
- `server.c` - Non-blocking server using select()
- `client.c` - Non-blocking client using select()

### poll() Implementation
- `server_poll.c` - Non-blocking server using poll()
- `client_poll.c` - Non-blocking client using poll()

### Documentation
- `COMPARISON.md` - Detailed poll() vs select() comparison
- `Makefile` - Build all versions

## Quick Comparison

| Feature | select() | poll() |
|---------|----------|--------|
| FD Limit | 1024 (FD_SETSIZE) | No limit |
| API | Bitmasks (fd_set) | Array of structs |
| Performance | O(max_fd) | O(num_fds) |
| Portability | Excellent | Good |
| Error Detection | Basic | Advanced |

## Compilation

```bash
make
```

This builds:
- `server` (select version)
- `client` (select version)
- `server_poll` (poll version)
- `client_poll` (poll version)

## Usage

### Test select() Version

**Terminal 1:**
```bash
./server
```

**Terminal 2:**
```bash
./client 127.0.0.1
```

### Test poll() Version

**Terminal 1:**
```bash
./server_poll
```

**Terminal 2:**
```bash
./client_poll 127.0.0.1
```

### Multiple Clients

Open multiple terminals and connect several clients to see concurrent handling:
```bash
./client 127.0.0.1
./client 127.0.0.1
./client 127.0.0.1
```

## Key Differences in Code

### select() Approach
```c
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(server_fd, &readfds);
select(max_fd + 1, &readfds, NULL, NULL, &timeout);
if (FD_ISSET(fd, &readfds)) { /* handle */ }
```

### poll() Approach
```c
struct pollfd fds[MAX_CLIENTS];
fds[0].fd = server_fd;
fds[0].events = POLLIN;
poll(fds, nfds, timeout);
if (fds[0].revents & POLLIN) { /* handle */ }
```

## Technical Details

- **Port**: 8080
- **Max Clients**: 10
- **Buffer Size**: 1024 bytes
- **Timeout**: 1 second
- **Protocol**: TCP/IP

## When to Use Which?

### Use select() when:
- Maximum portability needed
- Small number of FDs (< 100)
- Windows compatibility required

### Use poll() when:
- Large number of FDs
- Better error detection needed
- Modern Linux/Unix only
- Cleaner code preferred

## Performance Testing

```bash
# Run server
./server_poll &

# Connect 50 clients
for i in {1..50}; do
    echo "Client $i" | ./client_poll 127.0.0.1 &
done
```

## Learn More

See `COMPARISON.md` for:
- Detailed technical comparison
- Performance benchmarks
- Memory usage analysis
- Modern alternatives (epoll, kqueue)

## Clean Up

```bash
make clean
```

## Repository

https://github.com/ramgithub-ai/c-program-demo
