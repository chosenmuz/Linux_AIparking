/**
 * 描述: 使用RC522 RFID读卡器读取RFID卡片信息
 * 修改版：适配STM32 RC522模块通信协议
 */

 #include <stdio.h>
 #include <assert.h>
 #include <fcntl.h> 
 #include <unistd.h>
 #include <termios.h> 
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <sys/ioctl.h>
 #include <sys/select.h>
 #include <stdlib.h>
 #include <signal.h>
 #include <unistd.h>
 #include <netdb.h>
 #include <string.h>
 #include <errno.h>
 #include <time.h>
 #include <stdbool.h>
 #include <pthread.h>
 
 #include "include/head.h"
 
 // 定义读卡器两种状态
 enum state{IN, OUT};
 enum state RFID_state;
 
 bool cardOn = false;
 void *waitting(void *arg)
 {
	 char r[] = {'-', '\\', '|', '/'};
	 for(int i=0;;i++)
	 {
		 fprintf(stderr, "\r%c", r[i%4]);
		 if(cardOn)
			 usleep(100*1000);
		 else
			 usleep(400*1000);
	 }
 }
 
 //初始化串口
 int init_tty(int fd)
 {
	 struct termios old_flags,new_flags;   //定义两个结构体
	 bzero(&new_flags,sizeof(new_flags));  //// 将new_flags结构体的内存清零 
	 
	 //1. 获取旧的属性
	 tcgetattr(fd,&old_flags);       //终端恢复到原来的状态
	 
	 //2. 设置原始模式
	 cfmakeraw(&new_flags);
	 
	 //3. 激活本地连接CLOCAL与接收使能CREAD的选项  忽略调制解调器控制线
	 new_flags.c_cflag |= CLOCAL | CREAD; 
	 
	 //4. 设置波特率 - 修改为115200以匹配RC522模块
	 cfsetispeed(&new_flags, B115200); 
	 cfsetospeed(&new_flags, B115200);
	 
	 //5. 设置数据位为8位
	 new_flags.c_cflag &= ~CSIZE; //清空原有的数据位
	 new_flags.c_cflag |= CS8;
	 
	 //6. 设置奇偶检验位
	 new_flags.c_cflag &= ~PARENB;
	 
	 //7. 设置一位停止位
	 new_flags.c_cflag &= ~CSTOPB;
	 
	 //8. 设置等待时间，最少接收字符个数
	 new_flags.c_cc[VTIME] = 0;
	 new_flags.c_cc[VMIN] = 1;
	 
	 //9. 清空串口缓冲区
	 tcflush(fd, TCIFLUSH);
	 
	 //10. 设置串口的属性到文件中
	 if(tcsetattr(fd, TCSANOW, &new_flags) != 0)
	 {
		 perror("设置串口失败");
		 exit(0);
	 }
	 
	 return 0;
 }
 
 // RC522校验函数 - 异或后取反
 unsigned char CheckValueOut(unsigned char *Buf, unsigned char len)
 {
	 unsigned char i;
	 unsigned char checkValue = 0;
	 
	 for(i=0; i<(len-1); i++)
	 {
		 checkValue ^= Buf[i];
	 }
	 checkValue = ~checkValue;
	 
	 if(Buf[len-1] == checkValue)
		 return 0x00;  // STATUS_OK
	 else 
		 return 0x01;  // STATUS_ERR
 }
 
 void usage(int argc, char **argv)
 {
	 if(argc != 2)
	 {
		 fprintf(stderr, "Usage: %s <tty>\n", argv[0]);
		 exit(0);
	 }
 }
 
 /*************************************************
 功能: 初始化卡片放置标志位，当检查到卡片后，flag置为false，当卡片移除后1秒，
 执行refresh标志，将flag标志从false重新置为true，flag为真意味着：卡片刚放上去。
 *************************************************/
 bool flag = true; 
 void refresh(int sig)
 {
	 // 卡片离开1秒后
	 flag = true;
 }
 
 void *in_out(void *arg)
 {
	 // 按回车切换
	 while(1)
	 {
		 RFID_state = IN;
		 fprintf(stderr, "\r当前状态：【入库】\n");
		 getchar();
 
		 RFID_state = OUT;
		 fprintf(stderr, "\r当前状态：【出库】\n");
		 getchar();
	 }
 }
 
 int main(int argc, char **argv)
 {
	 usage(argc, argv);
	 
	 // 设置定时器到达指定时间执行的函数
	 signal(SIGALRM, refresh);
 
	 printf("Before open FIFO\n");
 
	 // 准备好通信管道
	 int fifoIN  = open(RFID2SQLiteIN,  O_RDWR);
	 int fifoOUT = open(RFID2SQLiteOUT, O_RDWR);
 
	 printf("After open FIFO\n");
	 
	 if(fifoIN==-1 || fifoOUT==-1)
	 {
		 perror("RFID模块打开管道失败");
		 exit(0);
	 }
 
	 // 初始化串口
	 int fd = open(argv[1], O_RDWR | O_NOCTTY);
	 if(fd == -1)
	 {
		 printf("open %s failed: %s\n", argv[1], strerror(errno));
		 exit(0);
	 }
	 init_tty(fd);
 
	 // 将串口设置为非阻塞状态，避免第一次运行卡住的情况
	 long state = fcntl(fd, F_GETFL);
	 state |= O_NONBLOCK;
	 fcntl(fd, F_SETFL, state);
 
	 // 向主控程序回到本模块启动成功
	 sem_t *s = sem_open(SEM_OK, O_CREAT, 0666, 0);
	 if(s != SEM_FAILED) {
		 sem_post(s);
	 }
 
	 // 延迟一会儿启动
	 sleep(1);
 
	 // 创建线程
	 pthread_t tid;
	 pthread_create(&tid, NULL, in_out,   NULL);
	 pthread_create(&tid, NULL, waitting, NULL);
 
	 unsigned char rbuf[30]; // 接收缓冲区
	 unsigned int cardid = 0;
	 
	 while(1)
	 {
		 // 清空接收缓冲区
		 memset(rbuf, 0, sizeof(rbuf));
		 
		 // 读取串口数据
		 int n = read(fd, rbuf, sizeof(rbuf));
		 
		 // 如果有数据
		 if(n > 0)
		 {
			 // 检查是否是RC522模块的数据包格式
			 // 数据包格式: 0x04 0x0C 0x02 0x30 状态 卡号(4字节) 卡类型(2字节) 校验和
			 if(n >= 12 && rbuf[0] == 0x04 && rbuf[1] == 0x0C && rbuf[2] == 0x02 && rbuf[3] == 0x30)
			 {
				 // 验证校验和
				 if(CheckValueOut(rbuf, 12) == 0x00 && rbuf[4] == 0x00)
				 {
					 cardOn = true;
					 printf("get rfid ok!\n");
					 
					 // 提取卡号 (4字节)
					 cardid = (rbuf[5] << 24) | (rbuf[6] << 16) | (rbuf[7] << 8) | rbuf[8];
					 
					 printf("cardid: %u\n", cardid);
					 
					 if(cardid == 0 || cardid == 0xFFFFFFFF)
					 {
						 usleep(100000); // 等待100ms后再次尝试
						 continue;
					 }
					 
					 // flag为真意味着：卡片刚放上去
					 if(flag)
					 {
						 if(RFID_state == IN)
						 {
							 // 此时RFID_state是IN，代表是新车入库
							 int m = write(fifoIN, &cardid, sizeof(cardid));
							 if(m <= 0)
							 {
								 perror("发送卡号失败");
							 }
						 }
 
						 if(RFID_state == OUT)
						 {
							 // 此时RFID_state是OUT，代表是有车出库
							 int m = write(fifoOUT, &cardid, sizeof(cardid));
							 if(m <= 0)
							 {
								 perror("发送卡号失败");
							 }
						 }
 
						 flag = false;
					 }
 
					 // 设置定时器，1秒后执行refresh()函数
					 alarm(1);
				 }
			 }
			 else
			 {
				 cardOn = false;
			 }
		 }
		 else
		 {
			 cardOn = false;
		 }
		 
		 usleep(100000); // 每100ms检查一次
	 }
 
	 close(fd);
	 exit(0);
 } 