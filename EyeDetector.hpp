//
// Created by kovi on 2/15/17.
//

#ifndef SHADOW_EXEDETECTOR_HPP
#define SHADOW_EXEDETECTOR_HPP

#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

struct EyeDetectResult {
    const cv::Point leftEye, rightEye;
    const bool detected;
};

class EyeDetector {
public:
    static cv::Mat preprocessedImage;
    static cv::Mat eyeTemplate;

    static EyeDetectResult detectEyes(cv::Mat &image) {
        image.copyTo(imageToDetect);
        EyeDetector::createPreprocessedImg().copyTo(preprocessedImage);

        int result_cols = imageToDetect.cols - eyeTemplate.cols + 1;
        int result_rows = imageToDetect.rows - eyeTemplate.rows + 1;
        cv::Mat matchResult(result_rows, result_cols, CV_32FC1);
        static const int match_method = CV_TM_CCOEFF_NORMED;
        cv::matchTemplate(imageToDetect, eyeTemplate, matchResult, match_method);
        //cv::normalize(matchResult, matchResult, 0, 10, cv::NORM_MINMAX, -1, cv::Mat());

        double minVal;
        double maxVal;
        cv::Point minLoc;
        cv::Point maxLoc;
        cv::Point matchLoc1;
        cv::Point matchLoc2;

        cv::minMaxLoc(matchResult, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

        if (match_method == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED) {
            matchLoc1 = minLoc;
        } else {
            matchLoc1 = maxLoc;
        }

        fillUnusedAreas(matchResult, matchLoc1);

        cv::minMaxLoc(matchResult, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

        if (match_method == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED) {
            matchLoc2 = minLoc;
        } else {
            matchLoc2 = maxLoc;
        }
        return EyeDetectResult{matchLoc1, matchLoc2, true};
    }

private:
    static const int MAX_KERNEL_LENGTH = 5;
    static cv::Mat imageToDetect;

    static cv::Mat createPreprocessedImg() {
        for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2)
            cv::bilateralFilter(imageToDetect, preprocessedImage,  i, i*1.5, i);
        return preprocessedImage;
    }

    static void fillUnusedAreas(cv::Mat &matchResult, const cv::Point &matchLocation) {
        cv::Point eyeCenter = matchLocation + cv::Point(eyeTemplate.cols / 2, eyeTemplate.rows / 2);

        cv::Range aboveEye(0, std::max(0, eyeCenter.y - eyeTemplate.rows));
        matchResult.rowRange(aboveEye).setTo(0);

        cv::Range underEye(std::min(eyeCenter.y + eyeTemplate.rows, matchResult.rows),matchResult.rows);
        matchResult.rowRange(underEye).setTo(0);

        cv::Range eyeRows(std::max(0,matchLocation.y - eyeTemplate.rows /2), std::min(matchResult.rows, matchLocation.y + eyeTemplate.rows / 2 ));
        cv::Range eyeCols(std::max(0,matchLocation.x - eyeTemplate.cols /2), std::min(matchResult.cols, matchLocation.x + eyeTemplate.cols / 2 ));
        matchResult.rowRange(eyeRows).colRange(eyeCols).setTo(0);
    }
};

cv::Mat EyeDetector::imageToDetect;
cv::Mat EyeDetector::preprocessedImage;
cv::Mat EyeDetector::eyeTemplate = cv::imread("media/eye_pattern.png", CV_LOAD_IMAGE_GRAYSCALE);
#endif //SHADOW_EXEDETECTOR_HPP
