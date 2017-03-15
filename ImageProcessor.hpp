//
// Created by kovi on 2/15/17.
//

#ifndef SHADOW_IMAGEPROCESSOR_HPP
#define SHADOW_IMAGEPROCESSOR_HPP


#include "ImageSource.hpp"
#include "GameElement.hpp"
#include "FlowCalculator.hpp"

#include <opencv2/opencv.hpp>

class ImageProcessor : public GameElement {
public:
    float virtualMouseDeltaX = 0.0f, virtualMouseDeltaY = 0.0f, virtualMouseDeltaZ = 0.0f;
    cv::Mat captured, prevCaptured, auxImage;
    ImageSource *imgSource;

    ImageProcessor()
            : auxImage(180, 320, CV_8UC1) {}

    void init() {
        cv::namedWindow(ImageProcessWindowName, CV_WINDOW_AUTOSIZE);
        renderString("Loading, please waiit...");
        imgSource = new WebcamImageSource();
        imgSource->init(20, "media/img%05d.png");
    }

    void render() {
        imgSource->getImage(captured);
        if (!prevCaptured.data) prevCaptured = captured.clone();
        if (!mainAreaCalculator.isInitialized() || !clickAreaCalculator.isInitialized()) {
            clickAreaWidth = (int) (captured.cols * clickAreaWidthRatio);
            mainAreaCalculator.init(captured, prevCaptured, cv::Range(clickAreaWidth, captured.cols), cv::Range::all());
            clickAreaCalculator.init(captured, prevCaptured, cv::Range(0, clickAreaWidth), cv::Range::all(), 2000.0);
        }
        mainAreaCalculator.calculateFlow();
        clickAreaCalculator.calculateFlow();
        virtualMouseDeltaX += mainAreaCalculator.getActualFlow().x * 0.002;
        virtualMouseDeltaY += mainAreaCalculator.getActualFlow().y * 0.002;
        drawFlow();
        cv::imshow(ImageProcessWindowName, imageToShow);
        cv::waitKey(1);
        captured.copyTo(prevCaptured);
    }

    void renderString(const char *message) {
        auxImage.setTo(0);
        drawTextToCenter(auxImage, message);
        cv::imshow(ImageProcessWindowName, auxImage);
        cv::waitKey(1);
    }

private:
    const char *ImageProcessWindowName = "ImageProcess";
    float clickAreaWidthRatio = 0.3f;
    int clickAreaWidth = 0;
    cv::Mat imageToShow;
    FlowCalculator mainAreaCalculator, clickAreaCalculator;


    const cv::Scalar upColor = cv::Scalar(0, 0, 255);
    const cv::Scalar downColor = cv::Scalar(255, 0, 0);
    const cv::Scalar rightColor = cv::Scalar(0, 255, 255);
    const cv::Scalar leftColor = cv::Scalar(255, 255, 255);

    void drawFlow() {
        if (!imageToShow.data) {
            imageToShow = cv::Mat(prevCaptured.size(), CV_8UC3);
        }
        cv::cvtColor(prevCaptured, imageToShow, CV_GRAY2BGR);

        drawMainAreaFlow();

        drawClickAreaFlow();

        drawSeparatorLine();
    }

    void drawMainAreaFlow() {
        static const cv::Point2f mainAreaCenter = cv::Point2f(clickAreaWidth + (captured.cols - clickAreaWidth) / 2, captured.rows / 2);
        static const cv::Point2f mainAreaLeftTopPoint = cv::Point2f(clickAreaWidth, 0);
        drawBigFlowArrow(mainAreaCalculator.getActualFlow(), mainAreaCenter);
        drawSmallFlowArrows(mainAreaCalculator.getFlowVectors(), mainAreaLeftTopPoint);
    }

    void drawSeparatorLine() {
        cv::line(imageToShow, cv::Point2f(clickAreaWidth, 0), cv::Point2f(clickAreaWidth, imageToShow.rows), cv::Scalar(0, 255, 0), 2,
                 CV_AA,
                 0);
    }

    void drawClickAreaFlow() {
        static const cv::Point2f clickAreaCenter = cv::Point2f(clickAreaWidth / 2, captured.rows / 2);
        static const cv::Point2f clickAreaLeftTopPoint = cv::Point2f(0, 0);
        drawBigFlowArrow(clickAreaCalculator.getActualFlow(), clickAreaCenter);
        drawSmallFlowArrows(clickAreaCalculator.getFlowVectors(), clickAreaLeftTopPoint);
    }

    void drawBigFlowArrow(const cv::Point2f &accFlow, const cv::Point2f &position) {
        cv::Scalar drawColor = calculateColor(accFlow);
        const cv::Point2f end = position + accFlow;
        cv::arrowedLine(imageToShow, position, end, drawColor, 3, CV_AA, 0, 0.4);
    }

    void drawSmallFlowArrows(const cv::Mat &flowVectors, const cv::Point2f &leftTopPosition) {
        const int step = 20;
        cv::Scalar arrowColor(0, 0, 0);
        for (int i = 0; i < flowVectors.rows; i += step) {
            for (int j = 0; j < flowVectors.cols; j += step) {
                const cv::Point2f &fxy = flowVectors.at<cv::Point2f>(i, j);
                arrowColor = calculateColor(fxy);
                cv::Point2f arrowStart = cv::Point2f(j, i) + leftTopPosition;
                cv::Point2f arrowEnd = arrowStart + fxy;
                cv::arrowedLine(imageToShow, arrowStart, arrowEnd, arrowColor, 1, CV_AA, 0, 0.4);
            }
        }
    }

    cv::Scalar calculateColor(const cv::Point2f &direction) {
        cv::Scalar calculatedColor(0, 0, 0);
        if ((fabs(direction.x) < 0.00001) && (fabs(direction.y) < 0.00001))
            return calculatedColor;
        cv::Point2f normalized = direction * (1 / cv::norm(direction));

        if (normalized.x > 0)
            calculatedColor += normalized.x * rightColor;
        else
            calculatedColor -= normalized.x * leftColor;

        if (normalized.y > 0)
            calculatedColor += normalized.y * downColor;
        else
            calculatedColor -= normalized.y * upColor;

        return calculatedColor;
    }
};

#endif //SHADOW_IMAGEPROCESSOR_HPP
