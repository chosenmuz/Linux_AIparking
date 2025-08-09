#include "common.h"
#include "camera.h"
#include "jpeglib.h"

char formats[5][16] = {0};
struct v4l2_fmtdesc fmtdesc;
struct v4l2_format  fmt;
struct v4l2_capability cap;


int CAMERA_W, CAMERA_H;

int R[256][256];
int G[256][256][256];
int B[256][256];

// 获取摄像头格式信息（固定）
bool get_caminfo(int camfd)
{
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    bool ret = false;
    printf("当前摄像头所支持的格式有: \n");
    while((ret=ioctl(camfd, VIDIOC_ENUM_FMT, &fmtdesc)) == 0)
    {
        printf("[%d]", fmtdesc.index);
        sprintf(formats[fmtdesc.index]+0, "%c", (fmtdesc.pixelformat>>8*0)&0xFF);
        sprintf(formats[fmtdesc.index]+1, "%c", (fmtdesc.pixelformat>>8*1)&0xFF);
        sprintf(formats[fmtdesc.index]+2, "%c", (fmtdesc.pixelformat>>8*2)&0xFF);
        sprintf(formats[fmtdesc.index]+3, "%c", (fmtdesc.pixelformat>>8*3)&0xFF);

        printf("\"%s\"", formats[fmtdesc.index]);
        printf("（详细描述: %s）\n", fmtdesc.description);
        fmtdesc.index++;
        ret = true;
    }
    printf("\n");

    return ret;
}

// 获取摄像头格式信息（可调）
bool get_camfmt(int camfd)
{
    bzero(&fmt, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(camfd, VIDIOC_G_FMT, &fmt) == -1)
    {
        printf("获取摄像头格式信息失败: %s\n", strerror(errno));
        return false;
    }

    /*
    printf("摄像头当前的配置信息:\n");
    printf("分辨率: %d×%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);

    printf("像素格式: ");
    switch(fmt.fmt.pix.pixelformat)
    {
    case V4L2_PIX_FMT_MJPEG:
        printf("V4L2_PIX_FMT_MJPEG\n");
        break;
    case V4L2_PIX_FMT_JPEG:
        printf("V4L2_PIX_FMT_JPEG\n");
        break;
    case V4L2_PIX_FMT_MPEG:
        printf("V4L2_PIX_FMT_MPEG\n");
        break;
    case V4L2_PIX_FMT_MPEG1:
        printf("V4L2_PIX_FMT_MPEG1\n");
        break;
    case V4L2_PIX_FMT_MPEG2:
        printf("V4L2_PIX_FMT_MPEG2\n");
        break;
    case V4L2_PIX_FMT_MPEG4:
        printf("V4L2_PIX_FMT_MPEG4\n");
        break;
    case V4L2_PIX_FMT_H264:
        printf("V4L2_PIX_FMT_H264\n");
        break;
    case V4L2_PIX_FMT_XVID:
        printf("V4L2_PIX_FMT_XVID\n");
        break;
    case V4L2_PIX_FMT_RGB24:
        printf("V4L2_PIX_FMT_RGB24\n");
        break;
    case V4L2_PIX_FMT_BGR24:
        printf("V4L2_PIX_FMT_BGR24\n");
        break;
    case V4L2_PIX_FMT_YUYV:
        printf("V4L2_PIX_FMT_YUYV\n");
        break;
    case V4L2_PIX_FMT_YYUV:
        printf("V4L2_PIX_FMT_YYUV\n");
        break;
    case V4L2_PIX_FMT_YVYU:
        printf("V4L2_PIX_FMT_YVYU\n");
        break;
    case V4L2_PIX_FMT_YUV444:
        printf("V4L2_PIX_FMT_YUV444\n");
        break;
    case V4L2_PIX_FMT_YUV410:
        printf("V4L2_PIX_FMT_YUV410\n");
        break;
    case V4L2_PIX_FMT_YUV420:
        printf("V4L2_PIX_FMT_YUV420\n");
        break;
    case V4L2_PIX_FMT_YVU420:
        printf("V4L2_PIX_FMT_YVU420\n");
        break;
    case V4L2_PIX_FMT_YUV422P:
        printf("V4L2_PIX_FMT_YUV422P\n");
        break;
    default:
        printf("未知\n");
    }
    printf("\n");
    */

    return true;
}


// 获取摄像头设备的基本参数
bool get_camcap(int camfd)
{
    bzero(&cap, sizeof(cap));
    if(ioctl(camfd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        printf("获取摄像头基本信息失败: %s\n", strerror(errno));
        return false;
    }

    printf("驱动：%s\n", cap.driver);
    printf("显卡：%s\n", cap.card);
    printf("总线：%s\n", cap.bus_info);
    printf("版本：%d\n", cap.version);

    if((cap.capabilities&V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
    {
        printf("该设备为视频采集设备\n");
    }
    if((cap.capabilities&V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
    {
        printf("该设备支持流IO操作\n\n");
    }

    return true;
}

// 配置摄像头像素格式
void set_camfmt(int camfd)
{
    struct v4l2_format *tmp = calloc(1, sizeof(*tmp));
    bzero(tmp, sizeof(*tmp));

    // 设置视频设备的类型：CAPTURE即摄像头
    tmp->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // 尝试多种分辨率和格式组合
    int resolutions[][2] = {{640, 480}, {320, 240}, {800, 600}, {1024, 768}};
    int formats[] = {V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_JPEG};
    int fields[] = {V4L2_FIELD_NONE, V4L2_FIELD_INTERLACED, V4L2_FIELD_ANY};
    
    int res_count = sizeof(resolutions) / sizeof(resolutions[0]);
    int fmt_count = sizeof(formats) / sizeof(formats[0]);
    int field_count = sizeof(fields) / sizeof(fields[0]);
    
    printf("尝试配置摄像头格式...\n");
    
    for(int i = 0; i < res_count; i++) {
        for(int j = 0; j < fmt_count; j++) {
            for(int k = 0; k < field_count; k++) {
                tmp->fmt.pix.width = resolutions[i][0];
                tmp->fmt.pix.height = resolutions[i][1];
                tmp->fmt.pix.pixelformat = formats[j];
                tmp->fmt.pix.field = fields[k];
                
                printf("尝试 %dx%d, 格式: 0x%x, 场模式: %d\n", 
                       tmp->fmt.pix.width, tmp->fmt.pix.height, 
                       tmp->fmt.pix.pixelformat, tmp->fmt.pix.field);
                
                if(ioctl(camfd, VIDIOC_S_FMT, tmp) == 0) {
                    printf("成功设置摄像头格式: %dx%d\n", 
                           tmp->fmt.pix.width, tmp->fmt.pix.height);
                    free(tmp);
                    return;
                }
            }
        }
    }
    
    printf("所有格式尝试失败，使用默认设置\n");
    // 如果所有尝试都失败，使用最基本的设置
    tmp->fmt.pix.width = 640;
    tmp->fmt.pix.height = 480;
    tmp->fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    tmp->fmt.pix.field = V4L2_FIELD_ANY;
    
    if(ioctl(camfd, VIDIOC_S_FMT, tmp) == -1) {
        printf("ioctl() VIDIOC_S_FMT 最终失败: %s\n", strerror(errno));
        printf("请检查摄像头设备是否正确连接\n");
        free(tmp);
        exit(1);  // 摄像头初始化失败，退出程序
    }
    
    free(tmp);
}

/*************************************************
功能:提前计算好YUV-RGB之间的映射表，等需要使用到的时候直接查表
*************************************************/
void *convert(void *arg __attribute__((unused)))
{
    /*******************************
     R = Y + 1.042*(V-128);
     G = Y - 0.344*(U-128)-0.714*(V-128);
     B = Y + 1.772*(U-128);
    *******************************/
    pthread_detach(pthread_self());

    for(int i=0; i<256; i++)
    {
        for(int j=0; j<256; j++)
        {
            R[i][j] = i + 1.042*(j-128);
            R[i][j] = R[i][j]>255 ? 255 : R[i][j];
            R[i][j] = R[i][j]<0   ? 0   : R[i][j];

            B[i][j] = i + 1.772*(j-128);
            B[i][j] = B[i][j]>255 ? 255 : B[i][j];
            B[i][j] = B[i][j]<0   ? 0   : B[i][j];

            for(int k=0; k<256; k++)
            {
                G[i][j][k] = i + 0.344*(j-128)-0.714*(k-128);
                G[i][j][k] = G[i][j][k]>255 ? 255 : G[i][j][k];
                G[i][j][k] = G[i][j][k]<0   ? 0   : G[i][j][k];
            }
        }
    }
    pthread_exit(NULL);
}

// 抓拍一张图片
void yuv2jpg(uint8_t *yuv)
{
    // 准备RGB数据
    uint8_t *rgbdata = (uint8_t *)calloc(1, CAMERA_W * CAMERA_H * 3);

    uint8_t Y0, U;
    uint8_t Y1, V;

    int yuv_offset, rgb_offset;
    for(int i=0; i<CAMERA_H; i++)
    {
        for(int j=0; j<CAMERA_W; j+=2)
        {
            yuv_offset = ( CAMERA_W*i + j ) * 2;
            rgb_offset = ( CAMERA_W*i + j ) * 3;

            Y0 = *(yuv + yuv_offset + 0);
            U  = *(yuv + yuv_offset + 1);
            Y1 = *(yuv + yuv_offset + 2);
            V  = *(yuv + yuv_offset + 3);

            *(rgbdata + rgb_offset + 0) = R[Y0][V];
            *(rgbdata + rgb_offset + 1) = G[Y0][U][V];
            *(rgbdata + rgb_offset + 2) = B[Y0][U];

            *(rgbdata + rgb_offset + 3) = R[Y1][V];
            *(rgbdata + rgb_offset + 4) = G[Y1][U][V];
            *(rgbdata + rgb_offset + 5) = B[Y1][U];
        }
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);  ////初始化错误处理对象、并将其与解压对象绑定
    jpeg_create_compress(&cinfo);    //创建解码对象


    // 准备图片文件
    FILE *fp = fopen("/mnt/AIParking/Imx6ull_code/build/a.jpg", "w");
    if(fp == NULL)
    {
        printf("\r创建文件[a.jpg]失败:%s\n", strerror(errno));
        return;
    }

    jpeg_stdio_dest(&cinfo, fp);//指定图像文件

    cinfo.image_width  = CAMERA_W;
    cinfo.image_height = CAMERA_H;
    cinfo.input_components = 3;  //颜色通道数
    cinfo.in_color_space   = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];

    // 开始编码，并将jpg数据存入指定的图片文件中
    while(cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = rgbdata + cinfo.next_scanline * CAMERA_W * 3;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    free(rgbdata);
    fclose(fp);
}
