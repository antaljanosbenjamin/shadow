//
// Created by kovi on 2/15/17.
//

#ifndef SHADOW_IMAGESOURCE_H
#define SHADOW_IMAGESOURCE_H

#include <opencv2/opencv.hpp>

class ImageSource {
public:
    virtual bool init(int intParam = 0, const char *strParam = NULL) = 0;

    virtual bool getImage(cv::Mat &img) = 0;
};


class FileImageSource : public virtual ImageSource {
private:
    char filenameFormat[512];
    int fileNum;
    int firstFileNum;

public:
    FileImageSource() {
        filenameFormat[0] = 0;
    }

    bool init(int intParam, const char *strParam) {
        firstFileNum = fileNum = intParam;
        if (strParam) {
            strncpy(filenameFormat, strParam, sizeof(filenameFormat));
        }

        return true;
    }

    bool getImage(cv::Mat &img) {
        char actualFilename[sizeof(filenameFormat)];
        sprintf(actualFilename, filenameFormat, fileNum++);
        img = cv::imread(actualFilename, CV_LOAD_IMAGE_GRAYSCALE);
        if (!img.data) {
            if (fileNum == firstFileNum) throw actualFilename;

            fileNum = firstFileNum;
            if (!getImage(img)) {
                throw actualFilename;
            }
            return true;
        }
        return img.rows | img.cols;
    }
};


class WebcamImageSource : public virtual ImageSource {
    cv::VideoCapture cap;
    std::string webcamWindow = "WebcamWindow";
    cv::Mat rawImg, resizedImg, resizedAndFlippedImg, resizedAndFlippedGrayscaleImage;

    bool init(int intParam = 0, const char *strParam = NULL) {
        if (!cap.open(0))
            throw "Can't open camera!";
        return 0;
    }

    bool getImage(cv::Mat &img) {
        cap >> rawImg;
        cv::resize(rawImg, resizedImg, cv::Size(320, 180));
        cv::flip(resizedImg, resizedAndFlippedImg, 1);
        cv::cvtColor(resizedAndFlippedImg, resizedAndFlippedGrayscaleImage, CV_BGR2GRAY);
        resizedAndFlippedGrayscaleImage.copyTo(img);

        return true;
    }
};

#endif //SHADOW_IMAGESOURCE_H
