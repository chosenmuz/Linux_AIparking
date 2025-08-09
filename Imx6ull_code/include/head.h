#ifndef _HEAD_H
#define _HEAD_H

#include <time.h>
#include <wait.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>       // 添加信号处理头文件

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>     // 添加进程等待头文件

#include <linux/fb.h>
#include <linux/input.h>

#include <arpa/inet.h>

// 规定管道的名称
#define RFID2SQLiteIN  "/tmp/fifo1" // RFID卡读取到卡号，告诉数据库有车进来，对应卡号要入库
#define RFID2SQLiteOUT "/tmp/fifo2" // RFID卡读取到卡号，告诉数据库有车出库，对应卡号要从库里删除
//#define RFID2SQLiteIN  "/mnt/AIParking/Imx6ull_code/rfid2sqlite_in" // RFID卡读取到卡号，告诉数据库有车进来，对应卡号要入库
//#define RFID2SQLiteOUT "/mnt/AIParking/Imx6ull_code/rfid2sqlite_out" // RFID卡读取到卡号，告诉数据库有车出库，对应卡号要从库里删除
#define SQLite2Audio   "/tmp/fifo3" // 数据库模块通过该管道，给音频模块发送文本
#define Video2SQLite   "/tmp/fifo4"  // 视频模块识别出车牌号后会通过该管道发送到数据库模块

// 子进程启动成功与否的信号量的名称
#define SEM_OK        "sem1"


#define SEM_TAKEPHOTO "sem2"

#endif
