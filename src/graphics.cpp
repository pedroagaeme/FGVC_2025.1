#include <GL/glut.h>
#include <cmath>
#include "graphics.h"
#include "utils.h"
#include "Vector3.h"
#include "Matrix3.h"


void mouseClickCallback(int button, int state, int mouseX, int mouseY) {
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if(collectedPoints < 6) {
            collectedPoints++;
        }
        glutPostRedisplay();
    }
}

void passiveMouseMotion(int x, int y) {
    if(collectedPoints < 6) {
        mouseToWorldCoords(x, y, worldX, worldY);
        double distanceX = worldX;
        double distanceY = worldY;
        if(collectedPoints < 2) {
            distanceX -= offsetCircle1X;
            distanceY -= offsetCircle1Y;
            capDistance2D(distanceX, distanceY);
            markedPoints[collectedPoints] = std::make_tuple(
                distanceX,
                distanceY,
                offsetCircle1X,
                offsetCircle1Y
            );
        }
        else if(collectedPoints > 2 && collectedPoints < 5) {
            distanceX -= offsetCircle2X;
            distanceY -= offsetCircle2Y;
            capDistance2D(distanceX, distanceY);
            markedPoints[collectedPoints] = std::make_tuple(
                distanceX,
                distanceY,
                offsetCircle2X,
                offsetCircle2Y
            );
        }
        else {
            int lineNumber = (collectedPoints + 1) / 3 - 1;
            int offsetX, offsetY;
            offsetX = offsetY = 0;
            if(lineNumber == 0) {
                offsetX = offsetCircle1X;
                offsetY = offsetCircle1Y;
            }
            else {
                offsetX = offsetCircle2X;
                offsetY = offsetCircle2Y;
            }
            distanceX -= offsetX;
            distanceY -= offsetY;
            auto [zRotationAngle, direction] = lineBaseRotations[lineNumber];
            Vector3 lineLocalDistances = Matrix3::rotationZCos(zRotationAngle, !direction) * Vector3(distanceX, distanceY, 0);
            distanceX = lineLocalDistances[0];
            distanceY = lineLocalDistances[1];
            if(distanceX > circleRadius) {
                distanceX = circleRadius;
            }
            else if(distanceX < -circleRadius) {
                distanceX = -circleRadius;
            }
            distanceY = sqrt(pow(circleRadius, 2) - pow(distanceX, 2));
            Vector3 pointInLine = lineTransformations[lineNumber] * Vector3(distanceX, distanceY, 0);

            markedPoints[collectedPoints] = std::make_tuple(
                pointInLine[0],
                pointInLine[1],
                offsetX,
                offsetY
            );
        }
        drawablePoints = collectedPoints + 1;
        glutPostRedisplay();
    }
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
    glColor3f(0.5, 0.5, 0.5);
    for(int j = 0; j < drawablePoints; j++) {
        auto[px, py, offsetCircleX, offsetCircleY] = markedPoints[j];
        if((pow(circleRadius, 2) - pow(px, 2) - pow(py, 2)) < 0.5) {
            for(float i = 0; i < 2 * M_PI; i += 0.001) {
                int pxCircle = (7 * cos(i)) - px + offsetCircleX;
                int pyCircle = (7 * sin(i)) - py + offsetCircleY;
            glVertex2i(pxCircle, pyCircle);
        }
        }
        for(float i = 0; i < 2 * M_PI; i += 0.001) {
            int pxCircle = (7 * cos(i)) + px + offsetCircleX;
            int pyCircle = (7 * sin(i)) + py + offsetCircleY;
            glVertex2i(pxCircle, pyCircle);
        }
    }

    // draw line 1 projected onto first circle
    if(collectedPoints >= 2) {
        glColor3f(1.0, 1.0, 1.0);

        Vector3 points[2];
        for(int j = 0; j < 2; j++) {
            auto [xCoord, yCoord, offsetCircleX, offsetCircleY] = markedPoints[j];
            double dx = (xCoord);
            double dy = (yCoord);
            double norm = pow(circleRadius, 2) - pow(dx, 2) - pow(dy, 2);
            long double dz = 0;
            if(norm > 0.5) {
                dz = sqrt(norm);
            }
            points[j] = Vector3(dx, dy, dz);
        }

        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({points[0], points[1]});
        lineTransformations[0] = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
        lineBaseRotations[0] = std::make_tuple(zRotationAngle, clockwise);

        for(float i = 0; i < M_PI; i += 0.001) {
            localCoordPoint = Vector3(cos(i), sin(i), 0);
            globalCoordPoint = lineTransformations[0] * localCoordPoint;

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

    // draw line 1 projected onto first circle
    if(collectedPoints >= 5) {
        glColor3f(1.0, 1.0, 1.0);

        Vector3 points[2];
        for(int j = 3; j < 5; j++) {
            auto [xCoord, yCoord, offsetCircleX, offsetCircleY] = markedPoints[j];
            double dx = (xCoord);
            double dy = (yCoord);
            double norm = pow(circleRadius, 2) - pow(dx, 2) - pow(dy, 2);
            long double dz = 0;
            if(norm > 0.5) {
                dz = sqrt(norm);
            }
            points[j - 3] = Vector3(dx, dy, dz);
        }

        // draw line 2 projected onto second circle
        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({points[0], points[1]});
        lineTransformations[1] = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
        lineBaseRotations[1] = std::make_tuple(zRotationAngle, clockwise);

        for(float i = 0; i < M_PI; i += 0.001) {
            localCoordPoint = Vector3(cos(i), sin(i), 0);
            globalCoordPoint = lineTransformations[1] * localCoordPoint;

            x = (circleRadius * globalCoordPoint[0]) + offsetCircle2X;
            y = (circleRadius * globalCoordPoint[1]) + offsetCircle2Y;
            glVertex2i(x, y);
        }
    }

    glEnd();
    glFlush();
}
