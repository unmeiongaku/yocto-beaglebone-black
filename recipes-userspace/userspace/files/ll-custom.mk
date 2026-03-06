.PHONY := majorminor cleanmm

CC = gcc
APP = app

MM_SOURCE_NAME = user-major-minor
MM_PATH = /usr/userspace/device-character/major-minor/
MM_TARGET = $(APP)-$(MM_SOURCE_NAME)
MM_SRC = $(MM_SOURCE_NAME).c

majorminor:
	$(CC) $(MM_PATH)$(MM_SRC) -o $(MM_PATH)$(MM_TARGET)

cleanmm:
	rm -rf $(MM_PATH)$(MM_TARGET)
