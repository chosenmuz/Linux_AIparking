/////////////////////////////////////////////////////
//
//  Copyright(C), 2011-2023, GEC Tech. Co., Ltd.
//  File name: Video.c
//
//  Description: 摄像头图像处理
//  Author: 林世霖
//  微信公众号：秘籍酷
//
//  GitHub: gitee.com/vincent040   
//  Bug Report: 2437231462@qq.com
//
//////////////////////////////////////////////////////

#include "camera.h"
#include "common.h"
#include "head.h"
#include <stdint.h>
#include <unistd.h>

#define SCREENSIZE 800*480*4

#define MIN(a, b) \
	({ \
		typeof(a) _a = a; \
		typeof(b) _b = b; \
		(void)(&_a==&_b); \
		_a < _b ? _a : _b; \
	})

int redoffset  ;
int greenoffset;
int blueoffset ;

int lcd;
struct fb_var_screeninfo lcdinfo;
uint8_t *fb;

int SCREEN_W, SCREEN_H;
int CAMERA_W, CAMERA_H;

int R[256][256];
int G[256][256][256];
int B[256][256];

sem_t *s_ok;
sem_t *s_takePhoto;

int fifo;

uint8_t *gyuv;

/*************************************************
功能:调用alpr程序，对抓拍到的a.jpg图片识别出车牌号
*************************************************/
char *autoLicensePlateRecognize()
{
	// 准备一个存放车牌号码的数组lp
	static char lp[10];
	bzero(lp, 10);

	// 调用alpr识别a.jpg，识别结果将会被存储在license中
	system("cd /mnt/AIParking/Imx6ull_code/build && ./alpr a.jpg");

	// 打开license并读取车牌号到lp中
	int fd = open("/mnt/AIParking/Imx6ull_code/build/license", O_RDONLY);
	if(fd != -1) {
		read(fd, lp, 10);
		close(fd);
	}

	// 返回车牌字符串lp
	return lp;
}

/*************************************************
功能:等待数据库发出信号，抓拍一张图片，识别出车牌号，再把车牌号返回给数据库
*************************************************/
void *takePhoto(void *arg)
{
	while(1)
	{
		// 静静地等待数据库模块的信号量通知
		if(s_takePhoto != SEM_FAILED) {
			sem_wait(s_takePhoto); //p操作
		} else {
			sleep(1);  // 如果信号量不可用，等待1秒
			continue;
		}

		// 抓拍一张图片
		yuv2jpg(gyuv);

		// 识别
		char *lp = autoLicensePlateRecognize();

		// 将识别之后的车牌发给数据库模块
		if(strlen(lp) == 9)
		    write(fifo, lp, 9);
		else
			fprintf(stderr, "\r无效车牌！请调整车辆角度\n");
	}
}

// 居中显示的偏移量（只计算一次）
static int center_offset_x = 0;
static int center_offset_y = 0;
static int center_initialized = 0;

/*************************************************
功能:显示视频画面 - 居中显示，高帧率版本
*************************************************/
void display(uint8_t *yuv)
{
	gyuv = yuv;

	// 只在第一次计算居中偏移
	if(!center_initialized) {
		center_offset_x = (SCREEN_W - CAMERA_W) / 2;
		center_offset_y = (SCREEN_H - CAMERA_H) / 2;
		printf("居中显示: 偏移(%d, %d)\n", center_offset_x, center_offset_y);
		
		// 清屏一次
		bzero(fb, SCREEN_W * SCREEN_H * 4);
		center_initialized = 1;
	}

	uint8_t Y0, U, Y1, V;
	uint8_t *fbtmp = fb;

	// 直接计算起始地址，减少重复计算
	int lcd_start_offset = (SCREEN_W * center_offset_y + center_offset_x) * 4;

	for(int y = 0; y < CAMERA_H; y++)
	{
		int yuv_line_start = CAMERA_W * y * 2;
		int lcd_line_start = lcd_start_offset + SCREEN_W * y * 4;
		
		for(int x = 0; x < CAMERA_W; x += 2)
		{
			int yuv_offset = yuv_line_start + x * 2;
			int lcd_offset = lcd_line_start + x * 4;
			
			// 读取YUV数据
			Y0 = yuv[yuv_offset];
			U  = yuv[yuv_offset + 1];
			Y1 = yuv[yuv_offset + 2];
			V  = yuv[yuv_offset + 3];

			// 写入第一个像素
			fbtmp[lcd_offset + redoffset] = R[Y0][V];
			fbtmp[lcd_offset + greenoffset] = G[Y0][U][V];
			fbtmp[lcd_offset + blueoffset] = B[Y0][U];

			// 写入第二个像素
			fbtmp[lcd_offset + redoffset + 4] = R[Y1][V];
			fbtmp[lcd_offset + greenoffset + 4] = G[Y1][U][V];
			fbtmp[lcd_offset + blueoffset + 4] = B[Y1][U];
		}
	}
}

int main(int argc, char *argv[])
{
	// 打开LCD设备
	lcd = open("/dev/fb0", O_RDWR);
	if(lcd == -1)
	{
		perror("open \"/dev/fb0\" failed");
		exit(0);
	}

		// 获取LCD显示器的设备参数     譬如 LCD 的 X 轴分辨率、Y 轴分辨 率以及像素格式等信息，通过这些参数计算出 LCD 显示缓冲区的大小。
	ioctl(lcd, FBIOGET_VSCREENINFO, &lcdinfo);

	// 打印LCD参数信息
	printf("LCD参数:\n");
	printf("  分辨率: %dx%d\n", lcdinfo.xres, lcdinfo.yres);
	printf("  像素深度: %d bpp\n", lcdinfo.bits_per_pixel);
	printf("  红色偏移: %d\n", lcdinfo.red.offset);
	printf("  绿色偏移: %d\n", lcdinfo.green.offset);
	printf("  蓝色偏移: %d\n", lcdinfo.blue.offset);

	// 将LCD显示器的水平、垂直分辨率存储在变量中
	SCREEN_W = lcdinfo.xres;  //一行有多少个像素点，X 分辨率 int形   xres * yres * bits_per_pixel / 8 计算可得到整个显示缓存区的大 小
	SCREEN_H = lcdinfo.yres;   //bits_per_pixel 表示像素深度 bpp，即每一个像 素点使用多少个 bit 位来描述它的颜色

	fb = mmap(NULL, lcdinfo.xres* lcdinfo.yres * lcdinfo.bits_per_pixel/8,
			    PROT_READ | PROT_WRITE, MAP_SHARED, lcd, 0);   //* 将显示缓冲区映射到进程地址空间 应用程序便可直接对显示缓冲区操作  可读可写*/
	if(fb == MAP_FAILED)
	{
		perror("mmap failed");
		exit(0);
	}

	printf("LCD framebuffer映射成功\n");

	// 初始化时清屏一次
	bzero(fb,  lcdinfo.xres * lcdinfo.yres * 4);   //清0 内存

	// 获取RGB偏移量
	redoffset  = lcdinfo.red.offset/8;     
	greenoffset= lcdinfo.green.offset/8;
	blueoffset = lcdinfo.blue.offset/8;   //字节级偏移

	// ************************************************** //
	
	// 准备好YUV-RGB映射表
	pthread_t tid;
	pthread_create(&tid, NULL, convert, NULL);


	// 打开摄像头设备文件
	int camfd = open(argv[1], O_RDWR);
	if(camfd == -1)
	{
		printf("open %s faield: %s\n", argv[1], strerror(errno));
		exit(0);
	}

	// 配置摄像头的采集格式
	set_camfmt(camfd);
	get_camfmt(camfd); //获取格式

	// 将摄像头的图像宽度、高度存储在变量中
	CAMERA_W = fmt.fmt.pix.width;
	CAMERA_H = fmt.fmt.pix.height;

	// 设置即将要申请的摄像头缓存的参数
	int nbuf = 3;

	// 创建一个v4l2_requestbuffers结构体reqbuf，并将其初始化为0
	struct v4l2_requestbuffers reqbuf;
	bzero(&reqbuf, sizeof (reqbuf));
	// 将reqbuf的type字段设置为V4L2_BUF_TYPE_VIDEO_CAPTURE，表示请求视频捕获缓冲区
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	// 将reqbuf的memory字段设置为V4L2_MEMORY_MMAP，表示请求内存映射方式
	reqbuf.memory = V4L2_MEMORY_MMAP;
	// 将reqbuf的count字段设置为nbuf，表示请求的缓冲区数量，此时为3
	reqbuf.count = nbuf;

	// 使用该参数reqbuf来申请缓存
	ioctl(camfd, VIDIOC_REQBUFS, &reqbuf);

	// 根据刚刚设置的reqbuf.count的值，来定义相应数量的struct v4l2_buffer
	// 每一个struct v4l2_buffer对应内核摄像头驱动中的一个缓存
	struct v4l2_buffer buffer[nbuf];
	int length[nbuf];
	uint8_t *start[nbuf];

	for(int i=0; i<nbuf; i++)
	{
		bzero(&buffer[i], sizeof(buffer[i]));
		buffer[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer[i].memory = V4L2_MEMORY_MMAP;
		buffer[i].index = i;
		ioctl(camfd, VIDIOC_QUERYBUF, &buffer[i]); ///获取到对应index的缓存信息，此处主要利用length信息及offset信息来完成后面的mmap操作。

		length[i] = buffer[i].length;
		start[i] = mmap(NULL, buffer[i].length,	PROT_READ | PROT_WRITE,
				        MAP_SHARED,	camfd, buffer[i].m.offset);  //将每一个帧缓冲对应的映射区的起始地址保 存在start[i]

		ioctl(camfd , VIDIOC_QBUF, &buffer[i]); // 将缓冲区放入队列中  		1、在尚未开启摄像头取像之前，需要将空的缓存一一入队
		                                                       			//2、针对视频输入，出队的时候如果缓存没有数据，那么出队将阻塞
																		//3、虽然内核对这些内存的定义时“队列”，但实际上不按顺序“插队”也是可以的，但一般不那么做

	}

	// 启动摄像头数据采集
	enum v4l2_buf_type vtype= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(camfd, VIDIOC_STREAMON, &vtype);//开启视频采集

	struct v4l2_buffer v4lbuf;
	bzero(&v4lbuf, sizeof(v4lbuf));
	v4lbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lbuf.memory= V4L2_MEMORY_MMAP;   //取出FIFO缓存中已经采样的帧缓存

	// 创建并准备好和数据库通讯管道
	mkfifo(Video2SQLite, 0666);  // 创建管道文件
	fifo = open(Video2SQLite, O_RDWR);
	if(fifo == -1)
	{
		perror("打开管道失败");
		exit(0);
	}

    // 准备好向主控程序通知本模块启动成功的信号量
    s_ok        = sem_open(SEM_OK, O_CREAT, 0666, 0);

    //准备好与视频模块沟通的信号量
    s_takePhoto = sem_open(SEM_TAKEPHOTO, O_CREAT, 0666, 0);

	if(s_ok == SEM_FAILED || s_takePhoto == SEM_FAILED)
	{
		perror("打开信号量失败");
		// 继续运行，不退出
	}

	// 创建一个专门用于抓拍的线程
	pthread_t t;
	pthread_create(&t, NULL, takePhoto, NULL);
		
	// 向主控程序汇报当前模块已经准备好
	if(s_ok != SEM_FAILED) {
		sem_post(s_ok);
	}

	// 开始抓取摄像头数据并在屏幕播放视频
	int i=0;
	while(1)
	{
		// 从队列中取出填满数据的缓存
		v4lbuf.index = i%nbuf;
		ioctl(camfd , VIDIOC_DQBUF, &v4lbuf);

		display(start[i%nbuf]);

	 	// 将已经读取过数据的缓存块重新置入队列中 
		v4lbuf.index = i%nbuf;
		ioctl(camfd , VIDIOC_QBUF, &v4lbuf);

		i++;
	}

	return 0;
}

