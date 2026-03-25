CC=gcc
CLI_TARGET=ncm2dump
GUI_TARGET=ncm2dump_gui
INCLUDE=-I./include/
LIBS=-lws2_32 -luser32 -lgdi32 -lshell32 -lole32 -luuid -lcomctl32

# 源文件
CORE_SRC=src/aes.c src/dump_windows.c src/cJSON.c src/encode.c src/decode.c src/buffer.c src/thpool.c
CLI_SRC=$(CORE_SRC) src/ncm.c
GUI_SRC=$(CORE_SRC) gui/gui_main.c gui/gui_window.c gui/gui_handler.c gui/gui_worker.c gui/gui_table.c gui/resource.o

# 默认目标
all: cli gui

cli: $(CLI_TARGET)
	@echo "CLI 构建完成: $(CLI_TARGET).exe"

gui: $(GUI_TARGET)
	@echo "GUI 构建完成: $(GUI_TARGET).exe"

$(CLI_TARGET):
	$(CC) -o $(CLI_TARGET) $(INCLUDE) -O2 $(CLI_SRC) -lpthread

gui/resource.o: gui/resource.rc
	windres gui/resource.rc -o gui/resource.o

$(GUI_TARGET): gui/resource.o
	$(CC) -o $(GUI_TARGET) $(INCLUDE) -O2 -mwindows $(GUI_SRC) $(LIBS) -lpthread

clean:
	rm -rf $(GUI_TARGET) $(CLI_TARGET)
