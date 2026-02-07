#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/protocol.h"

SOCKET connect_to_namenode() {
    SOCKET sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return INVALID_SOCKET;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(NAMENODE_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return INVALID_SOCKET;
    return sock;
}

SOCKET connect_to_datanode(char *ip, int port) {
    SOCKET sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return INVALID_SOCKET;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return INVALID_SOCKET;
    return sock;
}

void put_file(char *local_path, char *remote_filename) {
    SOCKET nn_sock = connect_to_namenode();
    if(nn_sock == INVALID_SOCKET) { printf("Cannot connect to NameNode\n"); return; }
    
    PacketHeader hdr;
    hdr.type = MSG_CREATE_FILE;
    hdr.payload_len = sizeof(CreateFileRequest);
    send(nn_sock, (char*)&hdr, sizeof(PacketHeader), 0);
    
    CreateFileRequest req;
    strcpy(req.filename, remote_filename);
    req.total_size = 0; 
    send(nn_sock, (char*)&req, sizeof(CreateFileRequest), 0);
    
    CreateFileResponse resp;
    recv(nn_sock, (char*)&resp, sizeof(CreateFileResponse), 0);
    closesocket(nn_sock);
    
    if(!resp.success) { printf("Failed to create file\n"); return; }
    
    printf("Created. Uploading...\n");
    
    FILE *fp = fopen(local_path, "rb");
    if(!fp) { perror("fopen"); return; }
    
    char buffer[4096]; // Chunk size
    size_t bytes_read;
    
    nn_sock = connect_to_namenode();
    hdr.type = MSG_ALLOCATE_BLOCK;
    send(nn_sock, (char*)&hdr, sizeof(PacketHeader), 0);
    
    AllocateBlockRequest alloc_req;
    strcpy(alloc_req.filename, remote_filename);
    send(nn_sock, (char*)&alloc_req, sizeof(AllocateBlockRequest), 0);
    
    AllocateBlockResponse alloc_resp;
    recv(nn_sock, (char*)&alloc_resp, sizeof(AllocateBlockResponse), 0);
    closesocket(nn_sock);
    
    printf("Block %d at %s:%d\n", alloc_resp.block_id, alloc_resp.datanode_ip, alloc_resp.datanode_port);
    
    SOCKET dn_sock = connect_to_datanode(alloc_resp.datanode_ip, alloc_resp.datanode_port);
    if(dn_sock == INVALID_SOCKET) { printf("Cannot connect to DataNode\n"); return; }
    
    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    rewind(fp);
    
    hdr.type = MSG_WRITE_BLOCK;
    hdr.payload_len = filesize;
    send(dn_sock, (char*)&hdr, sizeof(PacketHeader), 0);
    
    send(dn_sock, (char*)&alloc_resp.block_id, sizeof(int), 0);
    
    while((bytes_read = fread(buffer, 1, 4096, fp)) > 0) {
        send(dn_sock, buffer, bytes_read, 0);
    }
    
    int ack;
    recv(dn_sock, (char*)&ack, sizeof(int), 0);
    printf("Upload Complete. Ack: %d\n", ack);
    
    closesocket(dn_sock);
    fclose(fp);
}

void list_files() {
    SOCKET nn_sock = connect_to_namenode();
    if(nn_sock == INVALID_SOCKET) return;
    
    PacketHeader hdr;
    hdr.type = MSG_LIST_FILES;
    send(nn_sock, (char*)&hdr, sizeof(PacketHeader), 0);
    
    int count;
    recv(nn_sock, (char*)&count, sizeof(int), 0);
    
    printf("--- Remote Files (%d) ---\n", count);
    for(int i=0; i<count; i++) {
        char fname[MAX_FILENAME];
        int blks;
        recv(nn_sock, fname, MAX_FILENAME, 0);
        recv(nn_sock, (char*)&blks, sizeof(int), 0);
        printf("- %s (%d blocks)\n", fname, blks);
    }
    closesocket(nn_sock);
}

void get_file(char *remote_filename, char *local_path) {
    SOCKET nn_sock = connect_to_namenode();
    if(nn_sock == INVALID_SOCKET) return;
    
    PacketHeader hdr;
    hdr.type = MSG_LOOKUP_FILE;
    send(nn_sock, (char*)&hdr, sizeof(PacketHeader), 0);
    
    LookupFileRequest req;
    strcpy(req.filename, remote_filename);
    send(nn_sock, (char*)&req, sizeof(LookupFileRequest), 0);
    
    int block_count;
    recv(nn_sock, (char*)&block_count, sizeof(int), 0);
    
    if (block_count < 0) {
        printf("File not found.\n");
        closesocket(nn_sock);
        return;
    }
    
    FILE *fp = fopen(local_path, "wb");
    if (!fp) { perror("fopen"); closesocket(nn_sock); return; }
    
    for(int i=0; i<block_count; i++) {
        BlockLocation loc;
        recv(nn_sock, (char*)&loc, sizeof(BlockLocation), 0);
        
        printf("Fetching Block %d from %s:%d...\n", loc.block_id, loc.datanode_ip, loc.datanode_port);
        
        SOCKET dn_sock = connect_to_datanode(loc.datanode_ip, loc.datanode_port);
        if (dn_sock == INVALID_SOCKET) {
            printf("Failed to connect to DataNode.\n");
            continue;
        }
        
        PacketHeader dnhdr;
        dnhdr.type = MSG_READ_BLOCK;
        send(dn_sock, (char*)&dnhdr, sizeof(PacketHeader), 0);
        send(dn_sock, (char*)&loc.block_id, sizeof(int), 0);
        
        int size;
        recv(dn_sock, (char*)&size, sizeof(int), 0);
        
        if (size > 0) {
            char *buf = malloc(size);
            int total = 0;
            while(total < size) {
                int r = recv(dn_sock, buf+total, size-total, 0);
                if(r <= 0) break;
                total += r;
            }
            fwrite(buf, 1, total, fp);
            free(buf);
        }
        closesocket(dn_sock);
    }
    
    printf("Download complete.\n");
    fclose(fp);
    closesocket(nn_sock);
}

int main(int argc, char *argv[]) {
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;
    #endif

    if (argc < 2) {
        printf("Usage: mdfs <command> [args]\n");
        printf("Commands:\n  put <local> <remote>\n  get <remote> <local>\n  ls\n");
        return 1;
    }

    if (strcmp(argv[1], "put") == 0) {
        if (argc < 4) { printf("Usage: mdfs put <local> <remote>\n"); return 1; }
        put_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "get") == 0) {
        if (argc < 4) { printf("Usage: mdfs get <remote> <local>\n"); return 1; }
        get_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "ls") == 0) {
        list_files();
    } else {
        printf("Unknown command\n");
    }

    #ifdef _WIN32
        WSACleanup();
    #endif
    return 0;
}
