.PHONY := bno055 cleanbno

CC = gcc
APP = app

BNO_SOURCE_NAME = user-bno055-device
BNO_PATH = /usr/userspace/devices/bno055/
BNO_TARGET = $(APP)-$(BNO_SOURCE_NAME)
BNO_SRC = $(BNO_SOURCE_NAME).c

bno055:
	$(CC) $(BNO_PATH)$(BNO_SRC) -o $(BNO_PATH)$(BNO_TARGET)

cleanbno:
	rm -rf $(BNO_PATH)$(BNO_TARGET)