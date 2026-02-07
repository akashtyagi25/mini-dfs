#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#define MAX_FILENAME 64
#define BLOCK_SIZE (2 * 1024 * 1024) // 2MB Blocks
#define NAMENODE_PORT 9000

// Message Types
typedef enum {
    // Client -> NameNode
    MSG_LOOKUP_FILE = 1,    // "Where is 'movie.mp4'?"
    MSG_CREATE_FILE,        // "I want to write 'movie.mp4'"
    MSG_ALLOCATE_BLOCK,     // "I need a new block for 'movie.mp4'"
    MSG_LIST_FILES,         // "ls"
    
    // DataNode -> NameNode
    MSG_HEARTBEAT,          // "I am alive"
    MSG_BLOCK_REPORT,       // "I have blocks [1, 2, 5]"

    // NameNode -> Client
    MSG_LOOKUP_RESPONSE,
    MSG_CREATE_RESPONSE,
    MSG_ALLOCATE_RESPONSE,
    MSG_LIST_RESPONSE,

    // Client -> DataNode
    MSG_WRITE_BLOCK,        // "Here is data for Block 1"
    MSG_READ_BLOCK,         // "Give me data for Block 1"
    
    // DataNode -> Client
    MSG_READ_RESPONSE,      // "Here is the data"
    MSG_WRITE_ACK           // "Data saved"
} MsgType;

// Generic Header
typedef struct {
    MsgType type;
    int payload_len;
} PacketHeader;

// --- Payloads ---

// MSG_CREATE_FILE
typedef struct {
    char filename[MAX_FILENAME];
    long total_size;
} CreateFileRequest;

// MSG_CREATE_RESPONSE
typedef struct {
    bool success;
    char message[64];
} CreateFileResponse;

// MSG_ALLOCATE_BLOCK
typedef struct {
    char filename[MAX_FILENAME];
} AllocateBlockRequest;

// MSG_ALLOCATE_RESPONSE
typedef struct {
    int block_id;
    int datanode_port; // Simplified: Just send one node for now
    char datanode_ip[16];
} AllocateBlockResponse;

// MSG_LOOKUP_FILE
typedef struct {
    char filename[MAX_FILENAME];
} LookupFileRequest;

// MSG_LOOKUP_RESPONSE (Simplified: returns first block location)
typedef struct {
    int block_id;
    int datanode_port;
    char datanode_ip[16];
} BlockLocation;

// MSG_HEARTBEAT
typedef struct {
    int port; // Which port this DataNode is listening on
    int free_space_mb;
} HeartbeatPayload;

#endif
