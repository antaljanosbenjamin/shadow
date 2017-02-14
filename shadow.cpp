
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
// MsWindows-on ez is kell
#include <windows.h>
#pragma warning(disable: 4996)
#endif // Win32 platform

#include "utility2d3d.hpp"

using namespace std;


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

    }

    bool getImage(cv::Mat &img) {
        cap >> rawImg;
        cv::resize(rawImg, resizedImg, cv::Size(320, 180));
        cv::flip(resizedImg, resizedAndFlippedImg, 1);
        cv::cvtColor(resizedAndFlippedImg, resizedAndFlippedGrayscaleImage, CV_BGR2GRAY);
        resizedAndFlippedGrayscaleImage.copyTo(img);
    }
};


class GameControl {
public:
    GameControl()
            : userControlOn(false), timeOfInitCompleted(0.0), timeSinceInitCompleted(0.0), timeGameStart(-1.0) {}

    void captureTimeSinceInitCompleted() {
        timeSinceInitCompleted = glutGet(GLUT_ELAPSED_TIME) / 1000.0 - timeOfInitCompleted + timeSkip;
    }

    void setTimeOfInitCompleted() {
        timeOfInitCompleted = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    }

    bool userControlOn;
    float timeSinceInitCompleted;
    float timeGameStart;
    const float timeIntro = 27.0;
    const float timeOfTransitionFromIntroEndToUserControl = 1.0;

private:
    float timeOfInitCompleted;
    const float timeSkip = 0.0;
} gameController;


class GameElement {
public:
    virtual void init() = 0;

    virtual void render() = 0;
};

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

class ImageProcessor : public GameElement {
public:
    float virtualMouseDeltaX = 0.0f, virtualMouseDeltaY = 0.0f;
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
} imageProcessor;


class Avatar : public GameElement {
public:
    void init() {
        spaceShip.read("media/TIEInterceptor.obj");
    }

    void render() {
        glPushMatrix();
        transform();
        spaceShip.render();
        glPopMatrix();
    }

private:
    Mesh spaceShip;

    void transform() {
        float t = gameController.timeSinceInitCompleted < gameController.timeIntro ? t = 0 : t = gameController.timeSinceInitCompleted -
                                                                                                 gameController.timeIntro;

        glTranslatef(-0.47, -2.6, 2.0);

        float percent;

        if (gameController.timeGameStart < 0.0) {
            percent = 1.0;
        }
        else {
            if (gameController.timeSinceInitCompleted - gameController.timeGameStart <
                gameController.timeOfTransitionFromIntroEndToUserControl) {
                percent = 1.0 - (gameController.timeSinceInitCompleted - gameController.timeGameStart);
            }
            else {
                percent = 0.0;
                gameController.userControlOn = true;
            }
        }

        if (gameController.userControlOn) {
            glTranslatef(imageProcessor.virtualMouseDeltaX, 0, -imageProcessor.virtualMouseDeltaY);
        }

        glTranslatef((1.0 - percent) * 0.47, (1.0 - percent) * 0.0, (1.0 - percent) * -2.0);
        glRotatef(percent * fmod(t * 13, 360), 1, 0, 0);
        glRotatef(percent * fmod(t * 29, 360), 0, 0, 1);
        glTranslatef(percent * 0.5 * sin(M_PI / 2.0 * min(t / 10.0, 1.0)), 0, 0);

        glRotatef(percent * 90, 0, 1, 0);
        glRotatef(-180, 0, 0, 1);
        glRotatef(90, 1, 0, 0);
        glScalef(0.5, 0.5, 0.5);
    }
} interceptor;


class CrawlerScroll : public GameElement {
public:
    float arrowTrollStartTimeSec;

    void init() {
        texBigscroll = createTextureFromFile("media/iitcg.png");

        for (int h = 0; h < numTrollPhases; h++) {
            char fname[32];
            sprintf(fname, "media/source%d.png", h);
            texTroll[h] = createTextureFromFile(fname);
        }

        arrowTrollStartTimeSec = -5.0;
        trollPhase = 0;
    }

    void render() {
        const float crawlerLeftX = -1.5, crawlerTopY = 0.0, crawlerRightX = 1.5, crawlerBottomY = -5.0;
        const float trollLeftX = -1.33, trollTopY = -1.685, trollRightX = -0.51, trollBottomY = -2.28;
        const GLfloat white[] = {1.0, 1.0, 1.0, 1.0};

        glDisable(GL_DEPTH_TEST);
        glNormal3f(0, 0, 1);
        glMaterialfv(GL_FRONT, GL_AMBIENT, white);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, texBigscroll);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 1.0);
        glVertex2f(crawlerLeftX, crawlerTopY);
        glTexCoord2f(1.0, 1.0);
        glVertex2f(crawlerRightX, crawlerTopY);
        glTexCoord2f(1.0, 0.0);
        glVertex2f(crawlerRightX, crawlerBottomY);
        glTexCoord2f(0.0, 0.0);
        glVertex2f(crawlerLeftX, crawlerBottomY);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, texTroll[trollPhase]);
        glBegin(GL_QUADS);
        glTexCoord2f(0.005, 0.995);
        glVertex2f(trollLeftX, trollTopY);
        glTexCoord2f(0.995, 0.995);
        glVertex2f(trollRightX, trollTopY);
        glTexCoord2f(0.995, 0.005);
        glVertex2f(trollRightX, trollBottomY);
        glTexCoord2f(0.005, 0.005);
        glVertex2f(trollLeftX, trollBottomY);
        glEnd();

        renderArrowAndCalculateTrollPhase(gameController.timeSinceInitCompleted);

        glDisable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
    }

private:
    int trollPhase;
    static const int numTrollPhases = 12;
    GLuint texBigscroll, texTroll[numTrollPhases];

    const float arrowTime = 3.0;
    const float trollTime = 1.5;

    void redArrow(float ratio) {
        const float vertices[][3] =
                {
                        {0.00,  -1.5 + 0.00,  0.00},
                        {0.00,  -1.5 + 0.045, 0.00},
                        {0.38,  -1.5 + 0.045, 0.00},
                        {0.38,  -1.5 + 0.00,  0.00},
                        {-1.25, -1.5 + 0.00,  0.00},
                        {-1.25, -1.5 - 0.045, 0.00},
                        {-1.60, -1.5 - 0.045, 0.00},
                        {-1.60, -1.5 + 0.00,  0.00},
                        {-1.25, -1.5 + 0.00,  0.00},
                        {-1.25, -1.5 + 0.80,  0.00},
                        {-1.70, -1.5 + 0.80,  0.00},
                };
        const int numVertices = sizeof(vertices) / sizeof(float[3]) - 1;

        GLfloat apex[3];
        int i;

        if (ratio <= 0.0) return;
        if (ratio > 1.0) ratio = 1.0;

        glDisable(GL_LIGHTING);
        glColor3f(1.0, 0.0, 0.0);
        glLineWidth(1.5);
        glPushMatrix();
        glTranslatef(1.08, -1.265, 0);
        glBegin(GL_LINE_STRIP);
        for (i = 0; i < numVertices; i++) {
            glVertex3fv(vertices[i]);
            float upscale = ratio * (numVertices);

            if ((int) upscale <= i) {
                float fraction = upscale - i;
                apex[0] = vertices[i][0] + (vertices[i + 1][0] - vertices[i][0]) * fraction;
                apex[1] = vertices[i][1] + (vertices[i + 1][1] - vertices[i][1]) * fraction;
                apex[2] = vertices[i][2] + (vertices[i + 1][2] - vertices[i][2]) * fraction;
                glVertex3fv(apex);
                break;
            }
        }
        glEnd();
        glTranslatef(apex[0], apex[1], apex[2]);
        glRotatef(180.0 * atan2((vertices[i + 1][1] - vertices[i][1]), (vertices[i + 1][0] - vertices[i][0])) / M_PI, 0, 0, 1);
        glBegin(GL_LINE_STRIP);
        glVertex3f(-0.03, 0.01, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(-0.03, -0.01, 0);
        glEnd();
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    void renderArrowAndCalculateTrollPhase(float actualTime) {
        trollPhase = 0;

        if (actualTime < arrowTrollStartTimeSec) return;
        if (actualTime > arrowTrollStartTimeSec + arrowTime + trollTime) return;
        if (actualTime <= arrowTrollStartTimeSec + arrowTime) redArrow((actualTime - arrowTrollStartTimeSec) / arrowTime);
        else trollPhase = 23.0 * (0.5 - fabs(0.5 - (actualTime - (arrowTrollStartTimeSec + arrowTime)) / trollTime));
    }
} crawler;

class Logo {
    char filename[512];
    cv::Mat image;
    static const int bytesPerPixel = 4;

public:

    void init(const char *sourceFile) {

        if (sourceFile) {
            strncpy(filename, sourceFile, sizeof(filename));
        }
        cv::Mat upsideDownImage = cv::imread(filename, CV_LOAD_IMAGE_UNCHANGED);
        cv::flip(upsideDownImage, image, 0);
    }

    void render() {
        glDrawPixels(image.cols, image.rows, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
    }
} logo;


void display(void) {
    static bool firstRun = true;
    if (firstRun) {
        firstRun = false;
        gameController.setTimeOfInitCompleted();
    }

    static float lastTime = 0;
    gameController.captureTimeSinceInitCompleted();

    if (gameController.userControlOn) {
        if (gameController.timeSinceInitCompleted - lastTime > 0.05) {
            lastTime = gameController.timeSinceInitCompleted;
            imageProcessor.render();
        }
    }
    else {
        char msg[256];
        if (gameController.timeSinceInitCompleted < gameController.timeIntro)
            sprintf(msg, "Game starts in %d seconds...",
                    int(gameController.timeIntro -
                        gameController.timeSinceInitCompleted + 1));
        else sprintf(msg, "Press Enter to play!");
        imageProcessor.renderString(msg);
    }

    const float speed = 1.0 / 7.0;
    const float stopInterval = 3.0;
    const float accel = speed / stopInterval;
    const float crawlTime = 32.0;
    const float gameStartPosition = (crawlTime + stopInterval) * speed - stopInterval * stopInterval * accel / 2.0;
    float globalCameraPosition;

    if (gameController.timeSinceInitCompleted < crawlTime) globalCameraPosition = gameController.timeSinceInitCompleted * speed;
    else if (gameController.timeSinceInitCompleted < crawlTime + stopInterval)
        globalCameraPosition =
                gameController.timeSinceInitCompleted * speed -
                (gameController.timeSinceInitCompleted - crawlTime) *
                (gameController.timeSinceInitCompleted - crawlTime) *
                accel / 2.0;
    else globalCameraPosition = gameStartPosition;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(0, -globalCameraPosition, 2.8, 0, 1 - globalCameraPosition, 0.8, 0, 0, 1);

    static const GLfloat lightColor[] = {1, 1, 1, 1.0};
    static const GLfloat lightPosition[4] = {0, -8, 20, 0};

    glShadeModel(GL_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    crawler.render();
    interceptor.render();
    logo.render();

    glutSwapBuffers();
}


void idle() {
    glutPostRedisplay();
}


void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat) w / (GLfloat) h, 0.1, 50.0);
}


void init(void) {
    imageProcessor.init();
    crawler.init();
    interceptor.init();
    logo.init("media/logo.png");
}


void keyboard(unsigned char key, int x, int y) {
    gameController.captureTimeSinceInitCompleted();

    switch (key) {
        case 27:
            exit(0);
            break;
        case ' ':
            crawler.arrowTrollStartTimeSec = gameController.timeSinceInitCompleted;
            break;
        case 13:
            //if ((gameController.timeSinceInitCompleted >= gameController.timeIntro) && (gameController.timeGameStart < 0.0))
        {
            gameController.timeGameStart = gameController.timeSinceInitCompleted - 1;
        }
            break;
        case 'w':
        case 'W':
            if (gameController.userControlOn) imageProcessor.virtualMouseDeltaY -= 0.05f;
            break;
        case 's':
        case 'S':
            if (gameController.userControlOn) imageProcessor.virtualMouseDeltaY += 0.05f;
            break;
        case 'a':
        case 'A':
            if (gameController.userControlOn) imageProcessor.virtualMouseDeltaX -= 0.05f;
            break;
        case 'd':
        case 'D':
            if (gameController.userControlOn) imageProcessor.virtualMouseDeltaX += 0.05f;
            break;
        default:
            break;
    }
}


int main(int argc, char **argv) {
    try {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
        glutInitWindowSize(600, 600);
        glutInitWindowPosition(420, 0);
        glutCreateWindow("Press space to feed the troll :)");
        init();
        glutIdleFunc(idle);
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboard);
        glutMainLoop();
    }
    catch (const char *filename) {
        printf("Can't find %s.\n", filename);
    }

    return 0;
}
