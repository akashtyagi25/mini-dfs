#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../common/protocol.h"

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#endif

// Usage: datanode <port> <storage_dir>

void handle_request(SOCKET client_sock, char *storage_dir) {
    PacketHeader header;
    int valread = recv(client_sock, (char*)&header, sizeof(PacketHeader), 0);
    if (valread <= 0) return;

    if (header.type == MSG_WRITE_BLOCK) {
        int block_id;
        recv(client_sock, (char*)&block_id, sizeof(int), 0);
        printf("Received Write Request for Block %d\n", block_id);
        
        char filepath[128];
        sprintf(filepath, "%s/blk_%d", storage_dir, block_id);
        
        FILE *fp = fopen(filepath, "wb");
        if (fp) {
            char buffer[4096];
            int remaining = header.payload_len;
            while (remaining > 0) {
                int to_read = (remaining > 4096) ? 4096 : remaining;
                int r = recv(client_sock, buffer, to_read, 0);
                if (r <= 0) break;
                fwrite(buffer, 1, r, fp);
                remaining -= r;
            }
            fclose(fp);
            printf("Block %d saved.\n", block_id);
        } else {
            perror("fopen");
        }
        
        int ack = 1;
        send(client_sock, (char*)&ack, sizeof(int), 0);
    }
    else if (header.type == MSG_READ_BLOCK) {
        int block_id;
        recv(client_sock, (char*)&block_id, sizeof(int), 0);
        printf("Reading Block %d\n", block_id);
        
        char filepath[128];
        sprintf(filepath, "%s/blk_%d", storage_dir, block_id);
        
        FILE *fp = fopen(filepath, "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            int size = ftell(fp);
            rewind(fp);
            
            send(client_sock, (char*)&size, sizeof(int), 0);
            
            char buffer[4096];
            int bytes_read;
            while((bytes_read = fread(buffer, 1, 4096, fp)) > 0) {
                send(client_sock, buffer, bytes_read, 0);
            }
            fclose(fp);
        } else {
            int size = -1;
            send(client_sock, (char*)&size, sizeof(int), 0);
        }
    }
    
    closesocket(client_sock);
}

// Heartbeat function (Simulated thread)
// in Windows, we would use CreateThread
DWORD WINAPI HeartbeatThread(LPVOID lpParam) {
    int port = *(int*)lpParam;
    while(1) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock == INVALID_SOCKET) { Sleep(3000); continue; }
        
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(NAMENODE_PORT);
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
            PacketHeader hdr;
            hdr.type = MSG_HEARTBEAT;
            hdr.payload_len = sizeof(HeartbeatPayload);
            send(sock, (char*)&hdr, sizeof(PacketHeader), 0);
            
            HeartbeatPayload hb;
            hb.port = port;
            hb.free_space_mb = 1000;
            send(sock, (char*)&hb, sizeof(HeartbeatPayload), 0);
        }
        closesocket(sock);
        Sleep(3000);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;
    #endif

    if (argc < 3) {
        printf("Usage: datanode <port> <storage_dir>\n");
        return 1;
    }

    int port = atoi(argv[1]);
    char *storage_dir = argv[2];
    
    mkdir(storage_dir, 0777);

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return 1;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) return 1;

    if (listen(server_fd, 3) < 0) return 1;

    printf("DataNode listening on port %d, storing in %s\n", port, storage_dir);
    
    // Start Heartbeat Thread
    CreateThread(NULL, 0, HeartbeatThread, &port, 0, NULL);

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) continue;
        handle_request(new_socket, storage_dir);
    }

    #ifdef _WIN32
        WSACleanup();
    #endif
    return 0;
}
