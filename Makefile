CC=gcc
TARGET=ncm2dump
INCLUDE=-I./include/
#LIBS=-L./lib/ -lbase64 -lcjson -lpthread
#SRC=$(wildcard ./src/*.c)
SRC=src/aes.c src/dump_windows.c src/ncm.c src/cJSON.c src/encode.c src/decode.c src/buffer.c
$TARGET:
	$(CC) -o $(TARGET) $(INCLUDE) -O2 $(SRC)

clean:
	rm -rf $(TARGET)
