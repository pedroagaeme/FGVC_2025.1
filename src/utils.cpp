#include "utils.h"
#include "graphics.h"
#include <cmath>

int collectedPoints = 0;
int drawablePoints = 0;
std::tuple<double, double, int, int> markedPoints[6] = {};
const int offsetCircle1X = -300;
const int offsetCircle1Y = 0;
const int offsetCircle2X = 300;
const int offsetCircle2Y = 0;
int circleRadius = 200;
int worldX, worldY;
double infinityThreshold = 0.05;
bool isIdealLine[2] = {};
Matrix3 lineTransformations[2] = {};
std::tuple<double, bool> lineBaseRotations[2] = {};
std::tuple<double, double> interactivePoint = {};
bool canDrawInteractivePoint = false;

// Dynamic window size tracking
int currentWindowWidth = INITIAL_WINDOW_WIDTH;
int currentWindowHeight = INITIAL_WINDOW_HEIGHT;
bool isFullscreen = false;
bool showSupportingLines = false;

void myInit(void) {
    glClearColor(0.0,0.0,0.0,1.0);
    glPointSize(1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(WORLD_LEFT,WORLD_RIGHT,WORLD_BOTTOM,WORLD_TOP);
}

void mouseToWorldCoords(int mouseX,int mouseY,int& worldX,int& worldY) {
    // Dynamic coordinate mapping that exactly matches the shader behavior
    float worldWidth = WORLD_RIGHT - WORLD_LEFT;   // 1560
    float worldHeight = WORLD_TOP - WORLD_BOTTOM;  // 840
    float aspectRatio = (float)currentWindowWidth / currentWindowHeight;
    float worldAspectRatio = worldWidth / worldHeight;
    
    // Convert mouse coords to normalized device coordinates (-1 to 1)
    float ndcX = (2.0f * mouseX / currentWindowWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY / currentWindowHeight);
    
    // Apply inverse of the shader's aspect ratio scaling
    if(aspectRatio > worldAspectRatio) {
        // Viewport is wider than world, shader scales X down, so we scale X back up
        ndcX *= (aspectRatio / worldAspectRatio);
    } else {
        // Viewport is taller than world, shader scales Y down, so we scale Y back up
        ndcY *= (worldAspectRatio / aspectRatio);
    }
    
    // Convert from NDC to world coordinates
    worldX = (int)(ndcX * (WORLD_RIGHT - WORLD_LEFT) * 0.5f);
    worldY = (int)(ndcY * (WORLD_TOP - WORLD_BOTTOM) * 0.5f);
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

bool checkInfinityPoint(double px, double py) {
    return ((pow(circleRadius, 2) - pow(px, 2) - pow(py, 2)) < infinityThreshold);
}

//check if (x1,y1) = (x2,y2) or (x1,y1) = (-x2,-y2)
bool checkLinePointsDifferent(const Vector3& point1, const Vector3& point2) {
    double d1 = calcNorm2d(point1.x - point2.x, point1.y - point2.y);
    double d2 = calcNorm2d(point1.x + point2.x, point1.y + point2.y);
    return (std::min(d1, d2) > infinityThreshold);
}

// Helper: Lift a 2D point on the circle to 3D (on the sphere)
Vector3 liftToSphere(double x, double y, double radius) {
    double norm = pow(radius, 2) - pow(x, 2) - pow(y, 2);
    double z = sqrt(norm);
    if(std::isnan(z)) z = 0;
    return Vector3(x, y, z);
}

// Helper: Get the two 3D points for a line from markedPoints
void getLinePoints(int startIdx, Vector3& p1, Vector3& p2, double radius) {
    auto [x1, y1, _, __] = markedPoints[startIdx];
    auto [x2, y2, ___, ____] = markedPoints[startIdx + 1];
    p1 = liftToSphere(x1, y1, radius);
    p2 = liftToSphere(x2, y2, radius);
}

// Helper: Draw a projected line on a circle
void drawProjectedLine(const Matrix3& transformation, float offsetX, float offsetY, float radius, double sinXval, Vector3 linecolor) {
    Vector3 localCoordPoint, globalCoordPoint;
    float vx, vy, x, y;
    std::vector<float> projBuf;
    std::vector<float> projOppBuf;
    // subdued gray for supporting lines
    // draw only the arc from 0..PI (half circle) to avoid drawing the diameter
    for(float i = 0; i < M_PI; i += 0.001) {
        localCoordPoint = Vector3(cos(i), sin(i), 0);
        globalCoordPoint = transformation * localCoordPoint;

        vx = (radius * globalCoordPoint[0]);
        vy = (radius * globalCoordPoint[1]);
        x = vx + offsetX;
        y = vy + offsetY;
        projBuf.push_back(x);
        projBuf.push_back(y);
        projBuf.push_back(linecolor[0]);
        projBuf.push_back(linecolor[1]);
        projBuf.push_back(linecolor[2]);

        if(sinXval <= 0.001) {
            x = -vx + offsetX;
            y = -vy + offsetY;
            projOppBuf.push_back(x);
            projOppBuf.push_back(y);
            projOppBuf.push_back(linecolor[0]);
            projOppBuf.push_back(linecolor[1]);
            projOppBuf.push_back(linecolor[2]);
        }
    }
    drawVertices(projBuf, GL_LINE_STRIP);
    projBuf.clear();

    if(sinXval <= 0.001 && !projOppBuf.empty()) drawVertices(projOppBuf, GL_POINTS);
    projOppBuf.clear();

}

Vector3 lineIntersection(const Vector3 &line1, const Vector3 &line2){
    Vector3 cross = line1.cross(line2);
    return cross.normalize()*circleRadius;
}

void reshapeCallback(int width, int height) {
    currentWindowWidth = width;
    currentWindowHeight = height;
    
    // Update viewport
    glViewport(0, 0, width, height);
    
    // Update projection matrix for traditional OpenGL calls (if any)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(WORLD_LEFT, WORLD_RIGHT, WORLD_BOTTOM, WORLD_TOP);
    glMatrixMode(GL_MODELVIEW);
    
    glutPostRedisplay();
}

void keyboardCallback(unsigned char key, int x, int y) {
    static int savedWindowX = 100, savedWindowY = 100;
    static int savedWindowWidth = INITIAL_WINDOW_WIDTH;
    static int savedWindowHeight = INITIAL_WINDOW_HEIGHT;
    
    switch(key) {
        case 'f':
        case 'F':
            if(!isFullscreen) {
                // Save current window position and size
                savedWindowX = glutGet(GLUT_WINDOW_X);
                savedWindowY = glutGet(GLUT_WINDOW_Y);
                savedWindowWidth = glutGet(GLUT_WINDOW_WIDTH);
                savedWindowHeight = glutGet(GLUT_WINDOW_HEIGHT);
                
                // Go fullscreen
                glutFullScreen();
                isFullscreen = true;
            } else {
                // Exit fullscreen
                glutReshapeWindow(savedWindowWidth, savedWindowHeight);
                glutPositionWindow(savedWindowX, savedWindowY);
                isFullscreen = false;
            }
            break;
        case 27: // ESC key
            if(isFullscreen) {
                glutReshapeWindow(savedWindowWidth, savedWindowHeight);
                glutPositionWindow(savedWindowX, savedWindowY);
                isFullscreen = false;
            }
            break;
        case 'q':
        case 'Q':
            exit(0);
            break;
        case 's':
        case 'S':
            if(collectedPoints >= 6) {
                showSupportingLines = !showSupportingLines;
                glutPostRedisplay();
            }
            break;
    }
}