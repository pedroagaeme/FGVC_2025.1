#include "utils.h"
#include <cmath>

int collectedPoints = 0;
int drawablePoints = 0;
std::tuple<double, double, int, int> markedPoints[6] = {};
int offsetCircle1X = -300;
int offsetCircle1Y = 0;
int offsetCircle2X = 300;
int offsetCircle2Y = 0;
int circleRadius = 200;
int worldX, worldY;
Matrix3 lineTransformations[2] = {};
std::tuple<double, bool> lineBaseRotations[2];


void myInit(void) {
    glClearColor(0.0,0.0,0.0,1.0);
    glPointSize(1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(WORLD_LEFT,WORLD_RIGHT,WORLD_BOTTOM,WORLD_TOP);
}

void mouseToWorldCoords(int mouseX,int mouseY,int& worldX,int& worldY) {
    worldX = (int)((mouseX * 1560.0f / 1366) - 780);
    worldY = (int)(420 - (mouseY * 840.0f / 768));
}

double calcNorm2d(double distanceX,double distanceY) {
    return sqrt(pow(distanceX,2)+pow(distanceY,2));
}

void capDistance2D(double& distanceX,double& distanceY) {
    double norm=calcNorm2d(distanceX,distanceY);
    if(norm>circleRadius) {
        distanceX*=circleRadius/norm;
        distanceY*=circleRadius/norm;
    }
}

void setMaxDistance2D(double& distanceX,double& distanceY) {
    double norm=calcNorm2d(distanceX,distanceY);
    distanceX*=circleRadius/norm;
    distanceY*=circleRadius/norm;
}
