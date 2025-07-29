#include <GL/glut.h>
#include <cmath>
#include "graphics.h"
#include "utils.h"
#include "Vector3.h"
#include "Matrix3.h"

void mouseClickCallback(int button, int state, int mouseX, int mouseY) {
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if(collectedPoints < 3) {
            collectedPoints++;
        }
        glutPostRedisplay();
    }
}

void passiveMouseMotion(int x, int y) {
    if(collectedPoints < 6) {
        mouseToWorldCoords(x, y, worldX, worldY);
        if(collectedPoints < 3) {
            double distanceX = worldX - offsetCircle1X;
            double distanceY = worldY - offsetCircle1Y;
            capDistance2D(distanceX, distanceY);

            markedPoints[collectedPoints] = std::make_tuple(
                offsetCircle1X + distanceX,
                offsetCircle1Y + distanceY
            );
            drawablePoints = collectedPoints + 1;
        }
    }
    glutPostRedisplay();
}

std::tuple<long double, bool, long double> calculateRotations(std::tuple<Vector3, Vector3> line) {
    auto [p1, p2] = line;
    Vector3 lineVector = p1.cross(p2);

    if(lineVector[2] < 0)
        lineVector = lineVector * -1;

    Vector3 infinityPoint = Vector3(-lineVector[1], lineVector[0], 0).normalize();

    long double zRotationAngle = infinityPoint.dot(Vector3(1, 0, 0));
    bool clockwise = (infinityPoint[1] < 0);

    long double xRotationAngle = (lineVector.cross(infinityPoint).normalize())[2];

    return std::make_tuple(zRotationAngle, clockwise, xRotationAngle);
}

// ---- Display ----
void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POINTS);

    float x, y;
    Vector3 localCoordPoint;
    Vector3 globalCoordPoint;

    // draw first circle
    glColor3f(0.2, 0.2, 0.2);
    for(float i = 0; i < 2 * M_PI; i += 0.001) {
        x = (circleRadius * cos(i)) + offsetCircle1X;
        y = (circleRadius * sin(i)) + offsetCircle1Y;
        glVertex2i(x, y);
    }

    // draw marked points
    glColor3f(1, 1, 1);
    for(int j = 0; j < drawablePoints; j++) {
        auto[px, py] = markedPoints[j];
        for(float i = 0; i < 2 * M_PI; i += 0.001) {
            int pxCircle = (10 * cos(i)) + px;
            int pyCircle = (10 * sin(i)) + py;
            glVertex2i(pxCircle, pyCircle);
        }
    }

    // draw line 1 projected onto first circle
    if(collectedPoints >= 2) {
        glColor3f(0.0, 1.0, 1.0);

        Vector3 points[2];
        for(int j = 0; j < 2; j++) {
            auto [xCoord, yCoord] = markedPoints[j];
            double dx = (xCoord - offsetCircle1X);
            double dy = (yCoord - offsetCircle1Y);
            float dz = sqrt(pow(circleRadius, 2) - pow(dx, 2) - pow(dy, 2));
            points[j] = Vector3(dx, dy, dz);
        }

        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({points[0], points[1]});


        for(float i = 0; i < M_PI; i += 0.001) {
            localCoordPoint = Vector3(cos(i), sin(i), 0);
            globalCoordPoint =
                Matrix3::rotationZCos(zRotationAngle, clockwise) *
                (Matrix3::rotationXSin(xRotationAngle) *
                localCoordPoint);

            x = (circleRadius * globalCoordPoint[0]) + offsetCircle1X;
            y = (circleRadius * globalCoordPoint[1]) + offsetCircle1Y;
            glVertex2i(x, y);
        }
    }

    // draw second circle
    glColor3f(0.2, 0.2, 0.2);
    for(float i = 0; i < 2 * M_PI; i += 0.001) {
        x = (circleRadius * cos(i)) + offsetCircle2X;
        y = (circleRadius * sin(i)) + offsetCircle2Y;
        glVertex2i(x, y);
    }

    glEnd();
    glFlush();
}
