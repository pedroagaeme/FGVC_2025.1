#ifndef UTILS_H
#define UTILS_H

#include <GL/glut.h>
#include <tuple>

const int WINDOW_WIDTH = 1366;
const int WINDOW_HEIGHT = 768;
const int WORLD_LEFT = -780;
const int WORLD_RIGHT = 780;
const int WORLD_BOTTOM = -420;
const int WORLD_TOP = 420;

extern int collectedPoints;
extern int drawablePoints;
extern std::tuple<float, float> markedPoints[6];
extern int offsetCircle1X, offsetCircle1Y;
extern int offsetCircle2X, offsetCircle2Y;
extern int circleRadius;
extern int worldX, worldY;

void myInit(void);
void mouseToWorldCoords(int mouseX, int mouseY, int& worldX, int& worldY);
double calcNorm2d(double distanceX, double distanceY);
void capDistance2D(double& distanceX, double& distanceY);

#endif
