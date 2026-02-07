@echo off
if not exist bin mkdir bin
if not exist obj\namenode mkdir obj\namenode
if not exist obj\datanode mkdir obj\datanode
if not exist obj\client mkdir obj\client

echo Building NameNode...
gcc -Isrc/common src/namenode/main.c -o bin/namenode.exe -lws2_32

echo Building DataNode...
gcc -Isrc/common src/datanode/main.c -o bin/datanode.exe -lws2_32

echo Building Client...
gcc -Isrc/common src/client/main.c -o bin/client.exe -lws2_32

echo Build Complete.
