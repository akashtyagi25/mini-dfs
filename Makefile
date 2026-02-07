CC = gcc
CFLAGS = -Wall -Wextra -Isrc/common -g
LDFLAGS = -lpthread

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Targets
all: directories namenode datanode client

directories:
	@mkdir -p $(OBJ_DIR)/common
	@mkdir -p $(OBJ_DIR)/namenode
	@mkdir -p $(OBJ_DIR)/datanode
	@mkdir -p $(OBJ_DIR)/client
	@mkdir -p $(BIN_DIR)

# --- NameNode ---
namenode: $(OBJ_DIR)/namenode/main.o
	$(CC) $(CFLAGS) -o $(BIN_DIR)/namenode $^ $(LDFLAGS)

$(OBJ_DIR)/namenode/main.o: $(SRC_DIR)/namenode/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- DataNode ---
datanode: $(OBJ_DIR)/datanode/main.o
	$(CC) $(CFLAGS) -o $(BIN_DIR)/datanode $^ $(LDFLAGS)

$(OBJ_DIR)/datanode/main.o: $(SRC_DIR)/datanode/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Client ---
client: $(OBJ_DIR)/client/main.o
	$(CC) $(CFLAGS) -o $(BIN_DIR)/client $^ $(LDFLAGS)

$(OBJ_DIR)/client/main.o: $(SRC_DIR)/client/main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean directories
