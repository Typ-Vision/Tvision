#pragma once
#include <QImage>
#include <QPainter>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
inline QImage chessboardFixture(bool perspective=false) {
    QImage image(700,520,QImage::Format_Grayscale8);image.fill(255);
    {QPainter painter(&image);for(int y=0;y<7;++y)for(int x=0;x<10;++x)if((x+y)%2==0)painter.fillRect(50+x*60,50+y*60,60,60,Qt::black);}
    if(!perspective)return image;
    std::vector<cv::Point2f> from{{50,50},{650,50},{650,470},{50,470}},to{{85,75},{590,40},{650,455},{40,480}};
    const auto h=cv::getPerspectiveTransform(from,to);cv::Mat input(image.height(),image.width(),CV_8UC1,image.bits(),image.bytesPerLine()),out;
    cv::warpPerspective(input,out,h,input.size(),cv::INTER_LINEAR,cv::BORDER_CONSTANT,cv::Scalar(255));
    return QImage(out.data,out.cols,out.rows,out.step,QImage::Format_Grayscale8).copy();
}
