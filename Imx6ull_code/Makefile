# 确保build目录存在
$(shell mkdir -p build)

CROSS = arm-buildroot-linux-gnueabihf-
CC = $(CROSS)gcc

CFLAGS += -I include/

# 基础库
BASE_LDFLAGS = -L lib -lpthread -Wl,-rpath='$$ORIGIN/../lib'

# SQLite相关库 - 改用系统库
SQLITE_LDFLAGS = $(BASE_LDFLAGS) -lsqlite3 -ldl

# JPEG相关库  
JPEG_LDFLAGS = $(BASE_LDFLAGS) -ljpeg

# 所有模块
all: build/main build/RFID build/SQLite build/Audio build/Video

# RFID模块 - 只需要基础库
build/RFID: RFID.c
	$(CC) $^ -o $@ $(CFLAGS) $(BASE_LDFLAGS)

# SQLite模块 - 需要SQLite库
build/SQLite: SQLite.c
	$(CC) $^ -o $@ $(CFLAGS) $(SQLITE_LDFLAGS)

# Audio模块 - 只需要基础库
build/Audio: Audio.c
	$(CC) $^ -o $@ $(CFLAGS) $(BASE_LDFLAGS)

# Video模块 - 需要JPEG库
build/Video: Video.c camera.c
	$(CC) $^ -o $@ $(CFLAGS) $(JPEG_LDFLAGS)

# main模块 - 只需要基础库
build/main: main.c
	$(CC) $^ -o $@ $(CFLAGS) $(BASE_LDFLAGS)

clean:
	$(RM) -f build/* 
	@echo "已清理build目录下的所有文件"
