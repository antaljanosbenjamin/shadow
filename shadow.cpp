
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
// MsWindows-on ez is kell
#include <windows.h>
#pragma warning(disable: 4996)
#endif // Win32 platform

#include "utility2d3d.hpp"
#include "GameElement.hpp"
#include "ImageProcessor.hpp"
#include "TGALoader.hpp"
#include "OBJReader.hpp"

using namespace std;

class GameControl {
public:
    GameControl()
            : userControlOn(false), timeSinceInitCompleted(0.0f), timeGameStart(-1.0f), timeOfInitCompleted(0.0f) {}

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

ImageProcessor imageProcessor;


class Avatar : public GameElement {
public:
    void init() {
        spaceShip.read("media/xwing/x-wing.obj");
    }

    void render() {
        glPushMatrix();
        transform();
        spaceShip.render();
        glPopMatrix();
    }

private:
    OBJReader spaceShip;

    void transform() {
        float t = ( gameController.timeSinceInitCompleted < gameController.timeIntro ) ? 0 : gameController.timeSinceInitCompleted -
                                                                                             gameController.timeIntro;
        glPushMatrix();
        glTranslatef(-1.75,0,0);
        float matKd[] = {1,0,0,1};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, matKd);
        glMaterialfv(GL_FRONT, GL_AMBIENT, matKd);
        glMaterialfv(GL_FRONT, GL_SPECULAR, matKd);
        glutSolidCube(0.5);
        glPopMatrix();
        glTranslatef(-0.47 + 1.75 * 4, 3.6, 2.0);

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
            glTranslatef(imageProcessor.virtualMouseDeltaX, -imageProcessor.virtualMouseDeltaZ, -imageProcessor.virtualMouseDeltaY);
        }

        percent = 0.0;

        glTranslatef((1.0 - percent) * 0.47, (1.0 - percent) * 0.0, (1.0 - percent) * -2.0);
        glRotatef(percent * fmod(t * 13, 360), 1, 0, 0);
        glRotatef(percent * fmod(t * 29, 360), 0, 0, 1);
        glTranslatef(percent * 0.5 * sin(M_PI / 2.0 * min(t / 10.0, 1.0)), 0, 0);

        glRotatef(percent * 90, 0, 1, 0);
        glRotatef(-180, 0, 0, 1);
        glRotatef(90, 1, 0, 0);
        glScalef(0.5, 0.5, 0.5);
        float scale = 1.0/100; //(1 / spaceShip.maxVertices)*8;
        glScalef(scale, scale, scale);
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
        case 'q':
        case 'Q':
            if (gameController.userControlOn) imageProcessor.virtualMouseDeltaZ -= 0.05f;
            break;
        case 'e':
        case 'E':
            if (gameController.userControlOn) imageProcessor.virtualMouseDeltaZ += 0.05f;
            break;
        default:
            break;
    }
}


int main(int argc, char **argv) {
    NS_TGALOADER::IMAGE image;
    std::cout << image.LoadTGA("media/xwing/ENGINE.tga");
    std::cout << " " << image.getWidth() << " " << image.getHeight() << " " << image.getBytesPerPixel() << std:: endl;
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
