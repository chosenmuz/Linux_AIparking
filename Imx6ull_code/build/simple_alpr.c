#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("Usage: %s <image_file>\n", argv[0]);
        return 1;
    }
    
    // 模拟车牌识别过程
    printf("正在识别图片: %s\n", argv[1]);
    
    // 模拟一些常见的车牌号
    char *sample_plates[] = {
        "粤B12345",
        "京A67890", 
        "沪C11111",
        "川A22222",
        "豫B33333"
    };
    
    // 随机选择一个车牌号
    srand(time(NULL));
    int index = rand() % 5;
    char *plate = sample_plates[index];
    
    // 将识别结果写入license文件
    FILE *fp = fopen("license", "w");
    if(fp) {
        fprintf(fp, "%s", plate);
        fclose(fp);
        printf("识别成功: %s\n", plate);
    } else {
        printf("写入结果文件失败\n");
        return 1;
    }
    
    return 0;
} 