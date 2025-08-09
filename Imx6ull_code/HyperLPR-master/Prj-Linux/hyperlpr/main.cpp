#include "include/Pipeline.h"
#include <time.h>
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc,char **argv)
{
    if(argc<2)
    {
        printf("请指定一张JPEG格式的包含车牌的照片\n");
        return 0;
    }

    pr::PipelinePR prc("model/cascade.xml",
    "model/HorizonalFinemapping.prototxt",
    "model/HorizonalFinemapping.caffemodel",
    "model/Segmentation.prototxt",
    "model/Segmentation.caffemodel",
    "model/CharacterRecognization.prototxt",
    "model/CharacterRecognization.caffemodel",
    "model/SegmenationFree-Inception.prototxt",
    "model/SegmenationFree-Inception.caffemodel");

    cv::Mat image = cv::imread(argv[1]);
    std::vector<pr::PlateInfo> res = prc.RunPiplineAsImage(image,pr::SEGMENTATION_FREE_METHOD);

    fstream f("license", ios::out);
               
    for(auto st:res) 
    {
        if(st.confidence>0.75)
            f.write(st.getPlateName().c_str(), st.getPlateName().length());
    }
}
