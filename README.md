# Mini-DFS: Run Guide (Windows)

Ye project ek simplified "Google File System" hai jo aapke local computer par distributed system simulate karta hai.

Follow these steps exactly to run and present the project.

---

## 1. Build The Project 🛠️
Sabse pehle code ko `.exe` files mein convert karna hoga.

1.  Project folder open karein command prompt mein.
2.  Run karein:
    ```powershell
    .\build.bat
    ```
    Isse `bin` folder banega jisme `namenode.exe`, `datanode.exe`, aur `client.exe` honge.

---

## 2. Start Servers (3 Alag Windows Chahiye) 🖥️🖥️🖥️

Aapko 3 alag-alag **PowerShell** ya **CMD** windows kholni hongi.

### Terminal 1: START NAMENODE (The Master)
Ye "Manager" hai jo sab control karega. **Is window ko khula rakhna aur logs dekhna.**
```powershell
.\bin\namenode.exe
```

### Terminal 2: START DATANODE (The Storage)
Ye "Godown" hai jahan file save hogi.
```powershell
.\bin\datanode.exe 8001 "C:\Temp\dn1"
```

### Terminal 3: START CLIENT (User)
Yahan se hum commands denge.

---

## 3. How to Upload & Download Files 📂

**Step A: Ek test file banao**
Client terminal mein:
```powershell
echo "My Secret Data" > secret.txt
```

**Step B: Upload (PUT) ⬆️**
File ko DFS par `secret.txt` naam se upload karo:
```powershell
.\bin\client.exe put secret.txt /cloud_secret.txt
```
*(Server bolega: "Created. Uploading... Block 1000 at 127.0.0.1:8001")*

**Step C: List Files (LS) 📋**
Check karo server pe kya hai:
```powershell
.\bin\client.exe ls
```
*(Output: "- /cloud_secret.txt (1 blocks)")*

**Step D: Download (GET) ⬇️**
File wapas download karo naye naam se:
```powershell
.\bin\client.exe get /cloud_secret.txt downloaded.txt
```
*(Check karo: `type downloaded.txt`, wahi same data hona chahiye)*

---

## 4. 🔥 FAULT TOLERANCE TEST (Teacher Ko Dikhane Ke Liye) 🔥

Step-by-step procedure to show "Self-Healing":

**Phase 1: Setup**
1.  **Terminal 1:** `.\bin\namenode.exe` chalao.
2.  **Terminal 2:** `.\bin\datanode.exe 8001 "C:\Temp\dn1"` chalao.
    *   NameNode bolega: *"Registered new DataNode: 127.0.0.1:8001"*

**Phase 2: Success**
3.  **Terminal 3 (Client):** Ek file upload karo.
    ```powershell
    echo "Data 1" > file1.txt
    .\bin\client.exe put file1.txt /success.txt
    ```
    *(Ye Success hoga kyunki DataNode zinda hai)*

**Phase 3: The Crash (Simulation)**
4.  **Terminal 2 par jao (DataNode)** aur `Ctrl + C` dabao ya window band kar do.
    *(Ab DataNode mar chuka hai)*
5.  **Wait 10 Seconds...** NameNode terminal dekho.
    *(NameNode bolega: "**ALERT: DataNode 127.0.0.1:8001 is DEAD**")*

**Phase 4: Failure Handling**
6.  **Terminal 3 (Client):** Ab nayi file upload karne ki koshish karo.
    ```powershell
    echo "Data 2" > file2.txt
    .\bin\client.exe put file2.txt /fail.txt
    ```
7.  **Result:** Client bolega *"ERROR: No active DataNodes found"* ya upload fail ho jayega.
    *(Ye proof hai ki NameNode ne dead node par data bhejna band kar diya!)*

---
