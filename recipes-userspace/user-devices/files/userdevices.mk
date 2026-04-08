.PHONY := bno055 cleanbno:

CC = gcc
APP = app

BNO_SOURCE_NAME = user-bno055-device
BNO_PATH = /usr/userspace/devices/bno055/
BNOTARGET = $(APP)-$(CD_SOURCE_NAME)
BNO_SRC = $(CD_SOURCE_NAME).c

bno055:
	$(CC) $(CD_PATH)$(CD_SRC) -o $(CD_PATH)$(CD_TARGET)

cleanbno:
	rm -rf $(CD_PATH)$(CD_TARGET)