.PHONY := bno055 bmp280 cleanbno cleanbmp

CC = gcc
APP = app

BNO_SOURCE_NAME = user-bno055-device
BNO_PATH = /usr/userspace/devices/sensors/
BNO_TARGET = $(APP)-$(BNO_SOURCE_NAME)
BNO_SRC = $(BNO_SOURCE_NAME).c

BMP_SOURCE_NAME = user-bmp280-device
BMP_PATH = /usr/userspace/devices/sensors/
BMP_TARGET = $(APP)-$(BMP_SOURCE_NAME)
BMP_SRC = $(BMP_SOURCE_NAME).c

bno055:
	$(CC) $(BNO_PATH)$(BNO_SRC) -o $(BNO_PATH)$(BNO_TARGET)

bmp280:
	$(CC) $(BMP_PATH)$(BMP_SRC) -o $(BMP_PATH)$(BMP_TARGET)

cleanbno:
	rm -rf $(BNO_PATH)$(BNO_TARGET)
cleanbmp:
	rm -rf $(BMP_PATH)$(BMP_TARGET)