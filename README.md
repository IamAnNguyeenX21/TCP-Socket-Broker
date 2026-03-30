# TCP Socket Broker

A multi-threaded TCP server implementing a publish-subscribe message broker with topic-based routing. Clients can subscribe to topics, publish messages, and receive messages from other publishers on the same topics.

## Features

- **Multi-threaded Architecture**: Each client connection is handled in a separate thread for concurrent operations
- **Topic-based Pub/Sub**: Support for up to 100 topics with flexible subscription management
- **Thread-safe Operations**: Uses pthread mutexes to protect shared resources and prevent race conditions
- **Linked List Routing Table**: Efficient client-to-topic mapping using linked list data structure
- **Dynamic Client Management**: Automatic client registration/deregistration on subscription changes
- **Real-time Message Forwarding**: Messages published by a client are forwarded to all other subscribers on that topic

## Use Cases

- **IoT Message Brokering**: Distribute sensor data from multiple sources to interested subscribers
- **Real-time Event Distribution**: Broadcast system events or notifications to interested clients
- **Distributed Chat System**: Multiple clients publishing messages to shared topics
- **Monitoring and Logging**: Centralized message routing for distributed monitoring systems
- **Real-time Data Streaming**: Distribute time-series data or live feeds to multiple consumers

## Architecture

### Components

- **Server (main.c)**: Multi-threaded TCP server that manages client connections and message routing
- **Client (client.c)**: Interactive TCP client with dual-threaded architecture for simultaneous sending and receiving
- **Routing Table (routing_table.h/c)**: Linked list-based topic management and client subscription tracking
- **Protocol**: Simple text-based protocol using hexadecimal command codes

### Message Protocol

| Command | Format | Description |
|---------|--------|-------------|
| Subscribe | `0x00<topic_id>` | Subscribe to a specific topic (1-99) |
| Unsubscribe | `0x01<topic_id>` | Unsubscribe from a specific topic |
| Publish | `<message>` | Publish message to all subscribed topics (auto-prefixed with topic IDs by client) |
| Exit | `exit` | Disconnect from the server |

**How Publishing Works:**
- Client maintains a local list of subscribed topics
- When user enters a message, the client automatically prepends each subscribed topic ID and sends the message to all topics
- Server receives formatted message and routes it to appropriate topic subscribers

**Example Protocol Flow:**
```
Subscribe to topic 5:           0x005
Subscribe to topic 7:           0x007
User types message:             Hello World
Client sends (auto-generated):  5Hello World
Client sends (auto-generated):  7Hello World
Unsubscribe from topic 5:       0x015
```

## Building

### Prerequisites

- GCC compiler
- POSIX-compliant system (Linux, macOS, or WSL on Windows)
- pthread library (included in standard C libraries)

### Build Commands

```bash
# Navigate to the app directory
cd app_multi_thread

# Build both server and client
make all

# Build only the server
make app

# Build only the client
make client

# Clean all compiled files
make clean
```

This will generate:
- `app` - The TCP socket broker server executable
- `client` - The TCP client executable

## Running the Application

### Starting the Server

```bash
# Run the server on port 8080
./app
```

**Expected Output:**
```
Hello from the main thread!
```

The server will now listen for incoming client connections on `localhost:8080`.

### Starting a Client

In a separate terminal:

```bash
# Connect to the server
./client
```

**Expected Output:**
```
--- KHỞI TẠO TCP CLIENT ---
Đã kết nối thành công đến Server!
Gõ tin nhắn để gửi (Gõ 'exit' để thoát).

Client >
```

### Client Usage Examples

#### Example 1: Subscribe to a Topic

```
Client > 0x001
Đăng ký topic 1

Client >
```

#### Example 2: Publish a Message to Single Topic

After subscribing to a topic:

**Client 1:**
```
Client > 0x001
Đăng ký topic 1

Client > Hello from Client 1!
```
Client automatically sends: `1Hello from Client 1!` to topic 1

**Client 2 (subscribed to topic 1):**
```
Server trả lời: Message from topic 1: 1Hello from Client 1!
```

#### Example 3: Multiple Subscriptions with Multi-Topic Publishing

```
Client > 0x001
Đăng ký topic 1

Client > 0x002
Đăng ký topic 2

Client > Hello to all subscribed topics!
```
Client automatically sends to ALL subscribed topics:
- `1Hello to all subscribed topics!` (to topic 1)
- `2Hello to all subscribed topics!` (to topic 2)

All clients subscribed to topics 1 or 2 will receive the message.

#### Example 4: Unsubscribe from a Topic

```
Client > 0x001
Đăng ký topic 1

Client > 0x002
Đăng ký topic 2

Client > Message sent to topics 1 and 2
(Sends to both topics)

Client > 0x011
Hủy đăng ký topic 1

Client > Message sent to topic 2 only
(Now only sends to topic 2, automatically prepends "2")
```

#### Example 5: Disconnect

```
Client > exit
Đang ngắt kết nối...
```

## Implementation Details

### Server Architecture

- **Main Thread**: Accepts incoming client connections and spawns worker threads
- **Worker Threads**: Each thread handles one client's lifecycle:
  - Receives commands (subscribe, unsubscribe, publish, exit)
  - Manages routing table entries
  - Forwards published messages to subscribers
  - Cleans up subscriptions on disconnect

### Client Architecture

- **Main Thread (Sending)**: Handles user input, maintains local subscription list, automatically prepends topic IDs and sends messages
- **Receive Thread**: Listens for incoming messages from the server and displays them to the user
- **Local Subscription List**: Array tracking all topics the client has subscribed to
- **Auto-routing**: When user types a message, client automatically sends it to ALL subscribed topics

### Client Message Handling

When a user enters text into the client, the behavior depends on the input:

1. **Subscribe Command** (`0x00<topic_id>`):
   - Client adds topic_id to its local subscription list
   - Sends command to server: `0x00<topic_id>`
   - Server registers client for that topic

2. **Unsubscribe Command** (`0x01<topic_id>`):
   - Client removes topic_id from its local subscription list
   - Sends command to server: `0x01<topic_id>`
   - Server unregisters client from that topic

3. **Regular Message**:
   - For each topic in the client's local subscription list:
     - Prepend topic ID to the message
     - Send to server (e.g., if subscribed to topics 1 and 3, message "Hello" becomes "1Hello" and "3Hello")
   - Server routes each message to all other subscribers on that topic

4. **Exit Command**:
   - Client sends "exit" to server
   - Server cleans up all subscriptions for that client
   - Connection closes

### Data Structures

```c
typedef struct routing_information {
    int client_fd;                    // Client socket file descriptor
    struct routing_information* next; // Next node in linked list
} routing_information;

typedef struct topic_entry {
    int topic_id;                     // Topic identifier
    routing_information** head;       // Pointer to subscriber list
    pthread_mutex_t mutex;            // Mutex for thread-safe access
} topic_entry;
```

### Key Features

- **Thread Safety**: Each topic has its own mutex to prevent concurrent access issues
- **Automatic Cleanup**: When a client disconnects, all subscriptions are automatically removed
- **Memory Efficiency**: Linked lists provide O(n) insertion/deletion without array size limitations per topic
- **Scalability**: Can support multiple concurrent clients with independent message flows
- **Client-side Subscription Tracking**: Each client maintains a local list of subscribed topics
- **Automatic Message Routing**: Client automatically prepends topic IDs to messages and sends to all subscribed topics

## Compiling with Extra Debugging

For debugging with additional warnings:

```bash
gcc -Wall -Wextra -pthread -g -o app routing_table.c main.c
gcc -Wall -Wextra -pthread -g -o client client.c
```

Use `-g` flag to include debugging symbols for GDB.

## System Testing

### Test 1: Basic Pub/Sub with Auto-routing

1. Start server: `./app`
2. Start client 1: `./client` → Subscribe to topic 1: `0x001`
3. Start client 2: `./client` → Subscribe to topic 1: `0x001`
4. Client 1 types message: `Test message`
5. Client 1 automatically sends `1Test message` to topic 1
6. Client 2 should receive the message on topic 1

### Test 2: Multiple Topic Subscriptions

1. Start server and two clients
2. Client 1 subscribes to topics 1, 2, 3: `0x001`, `0x002`, `0x003`
3. Client 1 types: `Hello`
4. Verify client 1 automatically sends to all three topics:
   - `1Hello` to topic 1
   - `2Hello` to topic 2
   - `3Hello` to topic 3

### Test 3: Dynamic Subscription Changes

1. Client subscribes to topic 1: `0x001`
2. Client types and sends message to topic 1
3. Client subscribes to topic 2: `0x002`
4. Client types: `Now both topics` - message sent to both topic 1 and 2
5. Client unsubscribes: `0x011`
6. Client types: `Only topic 2` - message only sent to topic 2

### Test 4: Concurrent Connections

1. Start 5+ clients simultaneously
2. Each subscribes to different topics
3. Verify each client's messages are only sent to their subscribed topics
4. Test message routing accuracy across all clients

## Performance Considerations

- **Topic Capacity**: 100 topics maximum (adjustable in code)
- **Clients per Topic**: Limited by system resources (file descriptors)
- **Message Size**: 1KB buffer per message (adjustable in code)
- **Throughput**: Depends on network I/O and system CPU

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Address already in use" | Wait 60 seconds or use a different port |
| "Connection refused" | Ensure server is running before starting client |
| "Invalid topic ID" | Topic IDs must be between 1-99 |
| "Thread creation failed" | Check system resource limits |
| Message received with topic ID prefix | This is normal - client auto-prepends topic IDs before sending |
| Message not received by other clients | Check that receiving client has subscribed to the same topic |
| No messages sent when typing | Verify client has subscribed to at least one topic using `0x00<topic_id>` |


