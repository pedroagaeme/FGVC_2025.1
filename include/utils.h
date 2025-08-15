#ifndef UTILS_H
#define UTILS_H
#include "Matrix3.h"

#include <GL/glew.h>
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
extern std::tuple<double, double, int ,int> markedPoints[6];
extern const int offsetCircle1X, offsetCircle1Y;
extern const int offsetCircle2X, offsetCircle2Y;
extern int circleRadius;
extern int worldX, worldY;
extern double infinityThreshold;
extern bool isIdealLine[2];
extern Matrix3 lineTransformations[2];
extern std::tuple<double, bool> lineBaseRotations[2];
extern std::tuple<double, double> interactivePoint;
extern bool canDrawInteractivePoint;
void myInit(void);
void mouseToWorldCoords(int mouseX, int mouseY, int& worldX, int& worldY);
double calcNorm2d(double distanceX, double distanceY);
void capDistance2D(double& distanceX, double& distanceY);
bool checkInfinityPoint(double px, double dy);
bool checkLinePointsDifferent(const Vector3& point1, const Vector3& point2);
Vector3 liftToSphere(double x, double y, double radius);
void getLinePoints(int startIdx, Vector3& p1, Vector3& p2, double radius);
// drawOpposite default true to preserve previous behavior; set false to avoid drawing opposite-side vertices
void drawProjectedLine(const Matrix3& transformation, float offsetX, float offsetY, float radius, bool drawOpposite = true);
Vector3 lineIntersection(const Vector3 &line1, const Vector3 &line2);
#endif
