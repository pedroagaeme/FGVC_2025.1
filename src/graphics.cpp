#include <GL/glut.h>
#include <cmath>
#include "graphics.h"
#include "utils.h"
#include "Vector3.h"
#include "Matrix3.h"

Vector3 putPointInRealLine(double distanceX, double distanceY, int offsetX, int offsetY, int lineNumber) {
    distanceX -= offsetX;
    distanceY -= offsetY;
    auto [zRotationAngle, direction] = lineBaseRotations[lineNumber];
    Vector3 lineLocalDistances = Matrix3::rotationZCos(zRotationAngle, !direction) * Vector3(distanceX, distanceY, 0);
    distanceX = lineLocalDistances[0];
    distanceY = lineLocalDistances[1];
    if(abs(distanceX) > circleRadius) {
        distanceX = circleRadius;
    }
    distanceY = sqrt(pow(circleRadius, 2) - pow(distanceX, 2));
    return lineTransformations[lineNumber] * Vector3(distanceX, distanceY, 0);
}

void mouseClickCallback(int button, int state, int mouseX, int mouseY) {
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if(collectedPoints < 3) {
            bool allPointsDifferent = true;
            for(int i = 0; i <= collectedPoints; i++) {
                auto [px, py, offsetPX, offsetPY] = markedPoints[i];
                for(int j = i + 1; j <= collectedPoints; j++) {
                    auto [qx, qy, offsetQX, offsetQY] = markedPoints[j];
                    if(!checkLinePointsDifferent(Vector3(px, py, 0), Vector3(qx, qy, 0))) {
                        allPointsDifferent = false;
                        break;
                    }
                }
            }
            if(allPointsDifferent) {
                collectedPoints++;
            }
        }
        else if(collectedPoints < 6) {
            bool allPointsDifferent = true;
            for(int i = 3; i <= collectedPoints; i++) {
                auto [px, py, offsetPX, offsetPY] = markedPoints[i];
                for(int j = i + 1; j <= collectedPoints; j++) {
                    auto [qx, qy, offsetQX, offsetQY] = markedPoints[j];
                    if(!checkLinePointsDifferent(Vector3(px, py, 0), Vector3(qx, qy, 0))) {
                        allPointsDifferent = false;
                        break;
                    }
                }
            }
            if(allPointsDifferent) {
                collectedPoints++;
            }
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
            Vector3 pointInLine;
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
            if(isIdealLine[lineNumber]) {
                distanceX = (distanceX - offsetX) * circleRadius;
                distanceY = (distanceY - offsetY) * circleRadius;
                capDistance2D(distanceX, distanceY);
                pointInLine = Vector3(distanceX, distanceY, 0);
            }
            else {
                pointInLine = putPointInRealLine(distanceX, distanceY, offsetX, offsetY, lineNumber);
            }
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
    else {
        mouseToWorldCoords(x, y, worldX, worldY);
        double distanceX = worldX;
        double distanceY = worldY;
        Vector3 pointInLine;
        if(isIdealLine[0]) {
            distanceX = (distanceX - offsetCircle1X) * circleRadius;
            distanceY = (distanceY - offsetCircle1Y) * circleRadius;
            capDistance2D(distanceX, distanceY);
            pointInLine = Vector3(distanceX, distanceY, 0);
        }
        else {
            pointInLine = putPointInRealLine(distanceX, distanceY, offsetCircle1X, offsetCircle1Y, 0);
        }
        interactivePoint = std::make_tuple(pointInLine[0], pointInLine[1]);
        canDrawInteractivePoint = true;
        glutPostRedisplay();
    }
}


std::tuple<double, bool, double> calculateRotations(std::tuple<Vector3, Vector3> line) {
    auto [p1, p2] = line;
    Vector3 lineVector = p1.cross(p2);

    if(lineVector[0] == 0 && lineVector[1] == 0) {
        return std::make_tuple(1, false, 0);
    }

    if(lineVector[2] < 0) {
        lineVector = lineVector * -1;
    }

    Vector3 infinityPoint = Vector3(-lineVector[1], lineVector[0], 0).normalize();

    double zRotationAngle = infinityPoint.dot(Vector3(1, 0, 0));
    bool clockwise = (infinityPoint[1] < 0);

    double xRotationAngle = (lineVector.cross(infinityPoint).normalize())[2];

    return std::make_tuple(zRotationAngle, clockwise, xRotationAngle);
}

// ---- Display ----
void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POINTS);

    float x, y, vx, vy;
    Vector3 localCoordPoint;
    Vector3 globalCoordPoint;

    // draw first circle
    glColor3f(0.2, 0.2, 0.2);
    for(float i = 0; i < 2 * M_PI; i += 0.001) {
        x = (circleRadius * cos(i)) + offsetCircle1X;
        y = (circleRadius * sin(i)) + offsetCircle1Y;
        glVertex2f(x, y);
    }

    // draw line 1 projected onto first circle
    if(collectedPoints >= 2) {
        glColor3f(1.0, 1.0, 1.0);

        Vector3 p1, p2;
        getLinePoints(0, p1, p2, circleRadius);

        if(p1[2] < infinityThreshold && p2[2] < infinityThreshold) {
            isIdealLine[0] = true;
        }

        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({p1, p2});
        lineTransformations[0] = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
        lineBaseRotations[0] = std::make_tuple(zRotationAngle, clockwise);

        drawProjectedLine(lineTransformations[0], offsetCircle1X, offsetCircle1Y, circleRadius);
    }

    // draw second circle
    glColor3f(0.2, 0.2, 0.2);
    for(float i = 0; i < 2 * M_PI; i += 0.001) {
        x = (circleRadius * cos(i)) + offsetCircle2X;
        y = (circleRadius * sin(i)) + offsetCircle2Y;
        glVertex2f(x, y);
    }

    // draw line 2 projected onto second circle
    if(collectedPoints >= 5) {
        glColor3f(1.0, 1.0, 1.0);

        Vector3 p1, p2;
        getLinePoints(3, p1, p2, circleRadius);

        if(p1[2] < infinityThreshold && p2[2] < infinityThreshold) {
            isIdealLine[1] = true;
        }

        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({p1, p2});
        lineTransformations[1] = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
        lineBaseRotations[1] = std::make_tuple(zRotationAngle, clockwise);

        drawProjectedLine(lineTransformations[1], offsetCircle2X, offsetCircle2Y, circleRadius);

        //drawProjectedLine(lineTransformations[1], offsetCircle1X, offsetCircle1Y, circleRadius);
    }

    int pointOrder;
    // draw marked points
    for(int j = 0; j < drawablePoints; j++) {
        float rgbValues[3] = {0.7, 0.7, 0.7};
        rgbValues[j % 3] = 0;
        auto [redValue, greenValue, blueValue] = rgbValues;
        glColor3f(redValue, greenValue, blueValue);
        auto[px, py, offsetCircleX, offsetCircleY] = markedPoints[j];
        if(checkInfinityPoint(px, py)) {
            for(float i = 0; i < 2 * M_PI; i += 0.001) {
                float pxCircle = (7 * cos(i)) - px + offsetCircleX;
                float pyCircle = (7 * sin(i)) - py + offsetCircleY;
            glVertex2f(pxCircle, pyCircle);
        }
        }
        for(float i = 0; i < 2 * M_PI; i += 0.001) {
            float pxCircle = (7 * cos(i)) + px + offsetCircleX;
            float pyCircle = (7 * sin(i)) + py + offsetCircleY;
            glVertex2f(pxCircle, pyCircle);
        }
    }

    
    if(collectedPoints >= 6){
        // all points on the sphere
        Vector3 x1 = liftToSphere(std::get<0>(markedPoints[0]), std::get<1>(markedPoints[0]), circleRadius);
        Vector3 x2 = liftToSphere(std::get<0>(markedPoints[1]), std::get<1>(markedPoints[1]), circleRadius);
        Vector3 x3 = liftToSphere(std::get<0>(markedPoints[2]), std::get<1>(markedPoints[2]), circleRadius);
        Vector3 y1 = liftToSphere(std::get<0>(markedPoints[3]), std::get<1>(markedPoints[3]), circleRadius);
        Vector3 y2 = liftToSphere(std::get<0>(markedPoints[4]), std::get<1>(markedPoints[4]), circleRadius);
        Vector3 y3 = liftToSphere(std::get<0>(markedPoints[5]), std::get<1>(markedPoints[5]), circleRadius);
        
        // all lines between points (necessary for pappus line)
        Vector3 x1y2 = x1.cross(y2);
        Vector3 x2y1 = x2.cross(y1);
        Vector3 x1y3 = x1.cross(y3);
        Vector3 x3y1 = x3.cross(y1);


        Vector3 intersect1 = lineIntersection(x1y2, x2y1);
        Vector3 instersect2 = lineIntersection(x1y3, x3y1);

        Vector3 pappus = intersect1.cross(instersect2);
        
    // // Draw x1y2
    // {
    //     auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({x1, y2});
    //     Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
    //     std::tuple<double, bool> rotate = std::make_tuple(zRotationAngle, clockwise);
    //     drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);
    // }


    // // Draw x2y1
    // {
    //     auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({y1, x2});
    //     Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
    //     std::tuple<double, bool> rotate = std::make_tuple(zRotationAngle, clockwise);
    //     drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);
    // }

    //     // Draw x3y1
    // {
    //     auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({x3, y1});
    //     Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
    //     std::tuple<double, bool> rotate = std::make_tuple(zRotationAngle, clockwise);
    //     drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);
    // }


    //     // draw y3x1
    // {
    //     auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({y3, x1});
    //     Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
    //     std::tuple<double, bool> rotate = std::make_tuple(zRotationAngle, clockwise);
    //     drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);
    // }

    // {
    //     auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({x2, y3});
    //     Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
    //     std::tuple<double, bool> rotate = std::make_tuple(zRotationAngle, clockwise);
    //     drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);
    // }

    // {
    //     auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({y2, x3});
    //     Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
    //     std::tuple<double, bool> rotate = std::make_tuple(zRotationAngle, clockwise);
    //     drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);
    // }

    

        //draw pappus

        glColor3f(0.1, 0.1, 0.1);
        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({intersect1, instersect2});
        Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
        std::tuple<double, bool> rotate = std::make_tuple(zRotationAngle, clockwise);

        drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);
        drawProjectedLine(transform, offsetCircle2X, offsetCircle2Y, circleRadius);

        //draw interactive point
        glColor3f(1.0, 0.5, 0.0);
        if (canDrawInteractivePoint) {
            auto[px, py] = interactivePoint;
            if(checkInfinityPoint(px, py)) {
                for(float i = 0; i < 2 * M_PI; i += 0.001) {
                    float pxCircle = (7 * cos(i)) - px + offsetCircle1X;
                    float pyCircle = (7 * sin(i)) - py + offsetCircle1Y;
                glVertex2f(pxCircle, pyCircle);
                }
            }
            for(float i = 0; i < 2 * M_PI; i += 0.001) {
                float pxCircle = (7 * cos(i)) + px + offsetCircle1X;
                float  pyCircle = (7 * sin(i)) + py + offsetCircle1Y;
                glVertex2f(pxCircle, pyCircle);
            }

            // draw the image point
            Vector3 imageLine = y2.cross(y3);
            Vector3 itp = liftToSphere(px, py, circleRadius);
            Vector3 firstCorrrespondenceLine = y1.cross(itp);

            Vector3 pappusIntersection = lineIntersection(pappus, firstCorrrespondenceLine);
            Vector3 secondCorrrespondenceLine = x1.cross(pappusIntersection);

            Vector3 imagePoint = lineIntersection(imageLine, secondCorrrespondenceLine);

            if(imagePoint[2] < 0) {
                imagePoint = imagePoint * -1;
            }


            if (pappusIntersection[2] < 0) {
                pappusIntersection = pappusIntersection * -1;
            }
            
            glColor3f(0.1, 0.1, 0.1);
            auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({y1, itp});
            Matrix3 transform = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);

            drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius);

            auto [zRotationAngle2, clockwise2, xRotationAngle2] = calculateRotations({pappusIntersection, imagePoint});
            transform = Matrix3::rotationZCos(zRotationAngle2, clockwise2) * Matrix3::rotationXSin(xRotationAngle2);

            drawProjectedLine(transform, offsetCircle2X, offsetCircle2Y, circleRadius);

            auto [rx, ry, rz] = pappusIntersection;
            if(checkInfinityPoint(rx, ry)) {
                for(float i = 0; i < 2 * M_PI; i += 0.001) {
                    float pxCircle1 = (7 * cos(i)) - rx + offsetCircle1X;
                    float pyCircle1 = (7 * sin(i)) - ry + offsetCircle1Y;
                    float pxCircle2 = (7 * cos(i)) - rx + offsetCircle2X;
                    float pyCircle2 = (7 * sin(i)) - ry + offsetCircle2Y;
                    glVertex2f(pxCircle1, pyCircle1);
                    glVertex2f(pxCircle2, pyCircle2);
                }
            }
            for(float i = 0; i < 2 * M_PI; i += 0.001) {
                float  pxCircle1 = (7 * cos(i)) + rx + offsetCircle1X;
                float pyCircle1 = (7 * sin(i)) + ry + offsetCircle1Y;
                float pxCircle2 = (7 * cos(i)) + rx + offsetCircle2X;
                float pyCircle2 = (7 * sin(i)) + ry + offsetCircle2Y;
                glVertex2f(pxCircle1, pyCircle1);
                glVertex2f(pxCircle2, pyCircle2);
            }

            glColor3f(1.0, 0.5, 0.0);
            auto [qx, qy, qz] = imagePoint;
            if(checkInfinityPoint(qx, qy)) {
                for(float i = 0; i < 2 * M_PI; i += 0.001) {
                    float pxCircle = (7 * cos(i)) - qx + offsetCircle2X;
                    float pyCircle = (7 * sin(i)) - qy + offsetCircle2Y;
                glVertex2f(pxCircle, pyCircle);
                }
            }
            for(float i = 0; i < 2 * M_PI; i += 0.001) {
                float pxCircle = (7 * cos(i)) + qx + offsetCircle2X;
                float pyCircle = (7 * sin(i)) + qy + offsetCircle2Y;
                glVertex2f(pxCircle, pyCircle); 
            }
        }
    }

    glEnd();
    glFlush();
}
