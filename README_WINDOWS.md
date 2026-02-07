# Mini Distributed File System (Mini-DFS) - Windows Version

A simplified distributed file system implementation in C using Winsock2.

## Features
- **Distributed Storage**: Splits files into blocks and stores them on distinct DataNodes.
- **Fault Tolerance**: NameNode monitors DataNodes via Heartbeats. If a DataNode dies (stops sending heartbeats), the NameNode detects it within 10 seconds.
- **Dynamic Allocation**: Blocks are only allocated to live DataNodes.

## Prerequisites
- **MinGW GCC**: Ensure `gcc` is in your PATH.

## Build
Double click `build.bat` or run in terminal:
```cmd
.\build.bat
```

## Usage (Run in 3 Separate Powershell Windows)

### 1. Start NameNode (The Master)
```cmd
.\bin\namenode.exe
```
(Logs will show: "Registered new DataNode", "ALERT: DataNode DEAD", etc.)

### 2. Start DataNode (The Storage)
You can run multiple instances!
```cmd
.\bin\datanode.exe 8001 "C:\Temp\dn1"
```
```cmd
.\bin\datanode.exe 8002 "C:\Temp\dn2"
```

### 3. Use Client
```cmd
echo Hello > test.txt
.\bin\client.exe put test.txt /hello.txt
```

## Testing Fault Tolerance
1. Start NameNode and DataNode (Port 8001).
2. Upload a file correctly.
3. **Kill the DataNode** (Ctrl+C).
4. Watch the NameNode logs. After 10 seconds, it will say `ALERT: DataNode 127.0.0.1:8001 is DEAD`.
5. Try uploading another file -> It should fail (or pick another live DataNode if running).
