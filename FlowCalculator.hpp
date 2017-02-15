//
// Created by kovi on 2/15/17.
//

#ifndef FLOWCALCULATOR_HPP
#define FLOWCALCULATOR_HPP

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <opencv2/opencv.hpp>

class FlowCalculator {
public:

    void init(cv::Mat captured, cv::Mat prevCaptured, cv::Range horizontalRange, cv::Range verticalRange, float magicConstant = 5000.0) {
        if (captured.size != prevCaptured.size)
            throw "Captured and prevCaptured size must be the same!";
        this->captured = captured;
        this->prevCaptured = prevCaptured;
        this->horizontalRange = horizontalRange;
        this->verticalRange = verticalRange;
        this->initialized = true;
        this->magicConstant = magicConstant;
    }

    void init(cv::Mat captured, cv::Mat prevCaptured, float magicConstant = 5000.0) {
        init(captured, prevCaptured, cv::Range::all(), cv::Range::all(), magicConstant);
    }

    cv::Point2f calculateFlow() {
        cv::Mat ownPrevCaptured(prevCaptured, verticalRange, horizontalRange);
        cv::Mat ownCaptured(captured, verticalRange, horizontalRange);
        cv::Mat flowVectors2;
        cv::Point2f accFlow(0, 0);
        cv::calcOpticalFlowFarneback(ownPrevCaptured, ownCaptured, flowVectors, 0.5, 3, 15, 3, 5, 1.2, 0);
        cv::threshold(flowVectors, flowVectors2, 1.0, 0, cv::THRESH_TOZERO);
        cv::threshold(flowVectors, flowVectors, -1.0, 0, cv::THRESH_TOZERO_INV);
        flowVectors = flowVectors + flowVectors2;

        mainAreaSumFlow[sumFlowIndex] = cv::sum(flowVectors);
        mainAreaSumFlow[sumFlowIndex] /= magicConstant;
        sumFlowIndex = (sumFlowIndex + 1) % memoryLength;
        for (int i = 0; i < memoryLength; i++) {
            accFlow.x += mainAreaSumFlow[i][0];
            accFlow.y += mainAreaSumFlow[i][1];
        }

        if (fabs(accFlow.x) < 20.0) accFlow.x = 0;
        if (fabs(accFlow.y) < 20.0) accFlow.y = 0;

        actualFlow = accFlow;

        return accFlow;
    }

    const cv::Mat &getFlowVectors() const {
        return flowVectors;
    }

    const cv::Point2f &getActualFlow() const {
        return actualFlow;
    }

    bool isInitialized() const {
        return initialized;
    }

private:
    cv::Range horizontalRange, verticalRange;
    cv::Mat prevCaptured, captured;
    static const int memoryLength = 3;
    cv::Scalar mainAreaSumFlow[memoryLength];
    float magicConstant;
    int sumFlowIndex = 0;
    cv::Point2f actualFlow = cv::Point2f(0, 0);
    cv::Mat flowVectors;
    bool initialized;
};
#endif