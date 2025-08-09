#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

int main()
{
    printf("启动简化TTS服务器...\n");
    
    // 创建UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        perror("创建UDP端点失败");
        exit(1);
    }
    printf("创建UDP端点成功\n");

    // 设置地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(50001);

    // 绑定地址
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("绑定地址失败");
        exit(1);
    }
    printf("绑定地址成功，监听端口50001\n");

    char text[500];
    while(1) {
        // 接收文本
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        memset(text, 0, 500);
        
        int n = recvfrom(sockfd, text, 500, 0, (struct sockaddr *)&client_addr, &len);
        if(n > 0) {
            printf("收到【%s:%d】待合成文本: %s\n", 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port), 
                   text);

            // 使用espeak生成语音（如果系统有的话）
            char cmd[600];
            snprintf(cmd, 600, "espeak -v zh -s 150 -w a.wav \"%s\" 2>/dev/null || echo '模拟语音合成' > /dev/null", text);
            int ret = system(cmd);
            
            // 如果espeak失败，创建一个模拟的wav文件
            if(ret != 0) {
                // 创建一个简单的WAV文件头
                FILE *fp = fopen("a.wav", "wb");
                if(fp) {
                    // 简单的WAV头（44字节）
                    char wav_header[44] = {
                        'R','I','F','F', 0,0,0,0, 'W','A','V','E',
                        'f','m','t',' ', 16,0,0,0, 1,0, 1,0,
                        0x40,0x1f,0,0, 0x80,0x3e,0,0, 2,0, 16,0,
                        'd','a','t','a', 0,0,0,0
                    };
                    fwrite(wav_header, 1, 44, fp);
                    
                    // 写入一些静音数据
                    for(int i = 0; i < 8000; i++) {
                        short sample = 0;
                        fwrite(&sample, 2, 1, fp);
                    }
                    fclose(fp);
                }
                printf("生成模拟语音文件\n");
            } else {
                printf("使用espeak生成语音\n");
            }

            // 直接在Ubuntu PC上播放语音
            printf("在PC上播放语音: %s\n", text);
            system("aplay a.wav 2>/dev/null || paplay a.wav 2>/dev/null || echo '播放完成'");
            
            // 发送一个简单的确认给开发板
            uint32_t dummy_size = 100;
            sendto(sockfd, &dummy_size, sizeof(dummy_size), 0, 
                   (struct sockaddr *)&client_addr, len);
            
            // 发送少量数据确认
            char dummy_data[100] = "播放完成";
            sendto(sockfd, dummy_data, 100, 0, 
                   (struct sockaddr *)&client_addr, len);
            printf("语音播放完成\n\n");
        }
    }
    
    return 0;
} 