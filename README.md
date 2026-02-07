# Mini-DFS: Project Run Guide (Windows)

This project is a simplified **Distributed File System** (inspired by GFS/Hadoop) that simulates distributed storage on your local machine using standard TCP sockets.

Follow these steps exactly to run and demonstrate the project features.

---

## 1. Build The Project
First, compile the source code into .exe executables.

1.  Open the project folder in Command Prompt or PowerShell.
2.  Run the build script:
    ```powershell
    .\build.bat
    ```
    This will create a `bin` folder containing `namenode.exe`, `datanode.exe`, and `client.exe`.

---

## 2. Start Servers (Requires 3 Separate Windows)

You need to open 3 separate **PowerShell** or **CMD** windows.

### Terminal 1: START NAMENODE (The Master)
This is the central manager that tracks file locations and monitors DataNode health.
**Keep this window open to see logs.**
```powershell
.\bin\namenode.exe
```
*(Output: "NameNode listening on port 9000")*

### Terminal 2: START DATANODE (The Storage)
This is the worker node where actual file data is stored.
```powershell
.\bin\datanode.exe 8001 "C:\Temp\dn1"
```
*(Output: "DataNode listening... Registered new DataNode")*
*(Check the NameNode window - it should confirm the registration)*

### Terminal 3: START CLIENT (The User)
This is where we run commands to upload/download files.

---

## 3. How to Upload & Download Files

**Step A: Create a Test File**
In the Client terminal:
```powershell
echo "My Secret Data" > secret.txt
```

**Step B: Upload (PUT)**
Upload the file to the DFS as `cloud_secret.txt`:
```powershell
.\bin\client.exe put secret.txt /cloud_secret.txt
```
*(Server Log: "Created. Uploading... Block 1000 at 127.0.0.1:8001")*

**Step C: List Files (LS)**
Check what files are stored on the server:
```powershell
.\bin\client.exe ls
```
*(Output: "- /cloud_secret.txt (1 blocks)")*

**Step D: Download (GET)**
Download the file back to your local machine:
```powershell
.\bin\client.exe get /cloud_secret.txt downloaded.txt
```
*(Verify: `type downloaded.txt` should match the original content)*

---

## 4. FAULT TOLERANCE TEST (Use for Demo)

Step-by-step procedure to demonstrate the "Self-Healing" capability:

**Phase 1: Setup**
1.  **Terminal 1:** Run `.\bin\namenode.exe`.
2.  **Terminal 2:** Run `.\bin\datanode.exe 8001 "C:\Temp\dn1"`.
    *   NameNode says: *"Registered new DataNode: 127.0.0.1:8001"*

**Phase 2: Success Check**
3.  **Terminal 3 (Client):** Upload a file to verify system is working.
    ```powershell
    echo "Data 1" > file1.txt
    .\bin\client.exe put file1.txt /success.txt
    ```
    *(Success)*

**Phase 3: The Crash (Simulation)**
4.  **Go to Terminal 2 (DataNode)** and press `Ctrl + C` or close the window.
    *(The DataNode is now dead)*
5.  **Wait 10 Seconds...** Watch the NameNode terminal.
    *(NameNode Log: "**ALERT: DataNode 127.0.0.1:8001 is DEAD**")*

**Phase 4: Verify Failure Handling**
6.  **Terminal 3 (Client):** Try to upload a new file.
    ```powershell
    echo "Data 2" > file2.txt
    .\bin\client.exe put file2.txt /fail.txt
    ```
7.  **Result:** The Client will report an error or fail to upload because no active nodes are available.
    *(This proves the NameNode successfully detected the failure and stopped routing data to the dead node!)*
