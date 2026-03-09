.PHONY := characterdevice cleancd:

CC = gcc
APP = app

CD_SOURCE_NAME = user-character-device
CD_PATH = /usr/userspace/linux-learn/device-character/
CD_TARGET = $(APP)-$(CD_SOURCE_NAME)
CD_SRC = $(CD_SOURCE_NAME).c

characterdevice:
	$(CC) $(CD_PATH)$(CD_SRC) -o $(CD_PATH)$(CD_TARGET)

cleancd:
	rm -rf $(CD_PATH)$(CD_TARGET)
