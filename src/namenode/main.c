#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/protocol.h"

// In-memory File Table
typedef struct {
    char filename[MAX_FILENAME];
    int block_ids[100]; // Max 100 blocks per file for now
    int block_count;
} FileEntry;

#include <time.h>

// DataNode Registry
typedef struct {
    int port;
    char ip[16];
    time_t last_heartbeat;
    bool active;
} DataNodeEntry;

DataNodeEntry datanode_registry[10]; // Max 10 DataNodes
int datanode_count = 0;

void register_datanode(char *ip, int port) {
    for(int i=0; i<datanode_count; i++) {
        if(datanode_registry[i].port == port && strcmp(datanode_registry[i].ip, ip) == 0) {
            datanode_registry[i].last_heartbeat = time(NULL);
            datanode_registry[i].active = true;
            printf("Heartbeat from existing DN: %s:%d\n", ip, port);
            return;
        }
    }
    // New DataNode
    strcpy(datanode_registry[datanode_count].ip, ip);
    datanode_registry[datanode_count].port = port;
    datanode_registry[datanode_count].last_heartbeat = time(NULL);
    datanode_registry[datanode_count].active = true;
    datanode_count++;
    printf("Registered new DataNode: %s:%d\n", ip, port);
}

// Background thread to check for dead nodes
DWORD WINAPI MonitorThread(LPVOID lpParam) {
    while(1) {
        time_t now = time(NULL);
        for(int i=0; i<datanode_count; i++) {
            if(datanode_registry[i].active && difftime(now, datanode_registry[i].last_heartbeat) > 10) {
                printf("ALERT: DataNode %s:%d is DEAD (No heartbeat > 10s)\n", 
                       datanode_registry[i].ip, datanode_registry[i].port);
                datanode_registry[i].active = false;
                // Trigger Replication Logic here (Future Scope)
            }
        }
        Sleep(5000);
    }
    return 0;
}

FileEntry file_table[100]; // Max 100 files
int file_count = 0;

void handle_client(SOCKET client_sock) {
    PacketHeader header;
    int valread = recv(client_sock, (char*)&header, sizeof(PacketHeader), 0);
    if (valread <= 0) return;

    // printf("Received Request Type: %d\n", header.type); // Commented out to reduce spam

    if (header.type == MSG_HEARTBEAT) {
        HeartbeatPayload hb;
        recv(client_sock, (char*)&hb, sizeof(HeartbeatPayload), 0);
        register_datanode("127.0.0.1", hb.port); // Assuming localhost for demo
    }
    else if (header.type == MSG_CREATE_FILE) {
        CreateFileRequest req;
        recv(client_sock, (char*)&req, sizeof(CreateFileRequest), 0);
        printf("DEBUG: Creating file '%s'\n", req.filename);

        // Add to file table
        strcpy(file_table[file_count].filename, req.filename);
        file_table[file_count].block_count = 0;
        file_count++;

        // Send Response
        CreateFileResponse resp;
        resp.success = true;
        strcpy(resp.message, "File Created");
        
        send(client_sock, (char*)&resp, sizeof(CreateFileResponse), 0);
    }
    else if (header.type == MSG_ALLOCATE_BLOCK) {
        AllocateBlockRequest req;
        recv(client_sock, (char*)&req, sizeof(AllocateBlockRequest), 0);
        
        // Find file
        int found_idx = -1;
        for(int i=0; i<file_count; i++) {
            if(strcmp(file_table[i].filename, req.filename) == 0) {
                found_idx = i;
                break;
            }
        }
        
        if(found_idx != -1) {
            // Find a generic ACTIVE DataNode
            int target_dn_idx = -1;
            for(int k=0; k<datanode_count; k++) {
                if(datanode_registry[k].active) {
                    target_dn_idx = k;
                    break;
                }
            }
            
            AllocateBlockResponse resp;
            if (target_dn_idx != -1) {
                int new_blk_id = 1000 + file_table[found_idx].block_count;
                resp.block_id = new_blk_id;
                resp.datanode_port = datanode_registry[target_dn_idx].port;
                strcpy(resp.datanode_ip, datanode_registry[target_dn_idx].ip);
                
                file_table[found_idx].block_ids[file_table[found_idx].block_count++] = new_blk_id;
            } else {
                resp.block_id = -1; // No active DataNodes!
                printf("ERROR: No active DataNodes found for allocation!\n");
            }
            send(client_sock, (char*)&resp, sizeof(AllocateBlockResponse), 0);
        } else {
            AllocateBlockResponse resp;
            resp.block_id = -1;
            send(client_sock, (char*)&resp, sizeof(AllocateBlockResponse), 0);
        }
    }
    else if (header.type == MSG_LIST_FILES) {
        send(client_sock, (char*)&file_count, sizeof(int), 0);
        for(int i=0; i<file_count; i++) {
            send(client_sock, file_table[i].filename, MAX_FILENAME, 0);
            send(client_sock, (char*)&file_table[i].block_count, sizeof(int), 0);
        }
    }
    else if (header.type == MSG_LOOKUP_FILE) {
        LookupFileRequest req;
        recv(client_sock, (char*)&req, sizeof(LookupFileRequest), 0);
        
        int found_idx = -1;
        for(int i=0; i<file_count; i++) {
            if(strcmp(file_table[i].filename, req.filename) == 0) {
                found_idx = i;
                break;
            }
        }
        
        if (found_idx != -1) {
            int count = file_table[found_idx].block_count;
            send(client_sock, (char*)&count, sizeof(int), 0);
            
            for(int j=0; j<count; j++) {
                BlockLocation loc;
                loc.block_id = file_table[found_idx].block_ids[j];
                loc.datanode_port = 8001;
                strcpy(loc.datanode_ip, "127.0.0.1");
                send(client_sock, (char*)&loc, sizeof(BlockLocation), 0);
            }
        } else {
            int count = -1;
            send(client_sock, (char*)&count, sizeof(int), 0);
        }
    }
    
    closesocket(client_sock);
}

int main() {
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            printf("Failed. Error Code : %d", WSAGetLastError());
            return 1;
        }
    #endif

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(NAMENODE_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d", WSAGetLastError());
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return 1;
    }

    printf("NameNode listening on port %d\n", NAMENODE_PORT);
    
    // Start Monitor Thread
    CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) {
             printf("accept failed with error code : %d", WSAGetLastError());
            continue;
        }
        handle_client(new_socket);
    }
    
    #ifdef _WIN32
        WSACleanup();
    #endif
    return 0;
}
