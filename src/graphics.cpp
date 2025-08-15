#include <GL/glew.h>
#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include "graphics.h"
#include "utils.h"
#include "Vector3.h"
#include "Matrix3.h"

GLuint shaderProgram = 0;
static GLuint vao = 0, vbo = 0;
static size_t vboCapacityBytes = 0; // track current VBO allocation
// uniform locations for smoothing
static GLint uni_uAlpha = -1;
static GLint uni_uPointSize = -1;
static GLint uni_uIsPoint = -1;

// smoothing for interactive/mouse-driven visuals (render-only, not changing stored data)
static double drawMarkedX[6] = {0}, drawMarkedY[6] = {0};
static double targetMarkedX[6] = {0}, targetMarkedY[6] = {0};
static double drawInteractiveX = 0.0, drawInteractiveY = 0.0;
static double targetInteractiveX = 0.0, targetInteractiveY = 0.0;
static bool smoothingInitialized = false;
static const float smoothingFactor = 0.25f; // 0..1, larger = faster (less smooth)

// Improved vertex shader: expose point-size and use a viewport uniform for mapping
static const char* vertexShaderSrc = R"glsl(
#version 330 core
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inColor;
out vec3 fragColor;
uniform float uPointSize;
uniform int uIsPoint;
void main() {
    fragColor = inColor;
    // map world coords to clip space (world extents are +/-780 and +/-420)
    gl_Position = vec4(inPos.x / 780.0, inPos.y / 420.0, 0.0, 1.0);
    if(uIsPoint == 1) {
        gl_PointSize = uPointSize;
    }
}
)glsl";

// Improved fragment shader: support alpha and smooth round points using gl_PointCoord
static const char* fragmentShaderSrc = R"glsl(
#version 330 core
in vec3 fragColor;
out vec4 outColor;
uniform float uAlpha;
uniform int uIsPoint;
void main() {
    float alpha = uAlpha;
    // If rendering points, make them round and smooth using gl_PointCoord
    if(uIsPoint == 1) {
        // gl_PointCoord is only valid for points
        vec2 pc = gl_PointCoord.xy - vec2(0.5);
        float dist = length(pc);
        // smoothstep to create soft circular alpha (0.0..0.5 radius)
        float smoothA = smoothstep(0.5, 0.45, dist);
        alpha *= smoothA;
    }
    outColor = vec4(fragColor, alpha);
}
)glsl";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok) {
        char buf[1024];
        glGetShaderInfoLog(s, 1024, nullptr, buf);
        std::cerr << "Shader compile error: " << buf << std::endl;
    }
    return s;
}

void initGLResources() {
    // compile shaders
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    GLint ok;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
    if(!ok) {
        char buf[1024];
        glGetProgramInfoLog(shaderProgram, 1024, nullptr, buf);
        std::cerr << "Program link error: " << buf << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    // query uniform locations
    glUseProgram(shaderProgram);
    uni_uAlpha = glGetUniformLocation(shaderProgram, "uAlpha");
    uni_uPointSize = glGetUniformLocation(shaderProgram, "uPointSize");
    uni_uIsPoint = glGetUniformLocation(shaderProgram, "uIsPoint");
    // set sensible defaults
    if(uni_uAlpha != -1) glUniform1f(uni_uAlpha, 1.0f);
    if(uni_uPointSize != -1) glUniform1f(uni_uPointSize, 6.0f);
    if(uni_uIsPoint != -1) glUniform1i(uni_uIsPoint, 0);
    glUseProgram(0);

    // create VAO/VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // pre-allocate 1MB for streaming dynamic data to avoid frequent reallocations
    const size_t initialCapacity = 1024 * 1024; // bytes
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)initialCapacity, nullptr, GL_STREAM_DRAW);
    vboCapacityBytes = initialCapacity;

    // position (2 floats) + color (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 2));
    glBindVertexArray(0);

    // Enable blending and multisampling/line smoothing to reduce pixelated appearance
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    // Multisampling helps if the context supports it
    glEnable(GL_MULTISAMPLE);
}

// low-level helper: upload and draw one buffer as-is
static void drawRawVertices(const std::vector<float>& data, GLenum mode) {
    if(data.empty()) return;
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    size_t dataSizeBytes = data.size() * sizeof(float);
    // If the preallocated buffer is large enough, stream the data with BufferSubData to avoid reallocations
    if(dataSizeBytes <= vboCapacityBytes) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)dataSizeBytes, data.data());
    }
    else {
        // allocate larger buffer (grow) and update capacity
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)dataSizeBytes, data.data(), GL_STREAM_DRAW);
        vboCapacityBytes = dataSizeBytes;
    }

    // set smoothing uniforms depending on primitive type
    if(uni_uIsPoint != -1) {
        if(mode == GL_POINTS) glUniform1i(uni_uIsPoint, 1);
        else glUniform1i(uni_uIsPoint, 0);
    }
    if(uni_uAlpha != -1) {
        // keep full alpha for curves so colors remain unchanged
        glUniform1f(uni_uAlpha, 1.0f);
    }
    if(uni_uPointSize != -1 && mode == GL_POINTS) {
        // increase point size for better visibility and smoothing
        glUniform1f(uni_uPointSize, 6.0f);
    }

    GLsizei strideCount = (GLsizei)(data.size() / 5);
    glDrawArrays(mode, 0, strideCount);

    // reset point flag to avoid affecting subsequent draws
    if(uni_uIsPoint != -1) glUniform1i(uni_uIsPoint, 0);

    glBindVertexArray(0);
    glUseProgram(0);
}

void drawVertices(const std::vector<float>& data, GLenum mode) {
    if(data.empty()) return;
    if(mode != GL_LINE_STRIP) {
        drawRawVertices(data, mode);
        return;
    }

    const float splitThreshold = std::max(500.0f, (float)circleRadius * 2.0f);
    size_t vertCount = data.size() / 5;
    if(vertCount == 0) return;

    std::vector<float> segment;
    segment.reserve(5 * std::min((size_t)64, vertCount));

    auto pushVert = [&](size_t idx){
        segment.push_back(data[idx*5 + 0]);
        segment.push_back(data[idx*5 + 1]);
        segment.push_back(data[idx*5 + 2]);
        segment.push_back(data[idx*5 + 3]);
        segment.push_back(data[idx*5 + 4]);
    };

    // start with first vertex
    pushVert(0);
    for(size_t i = 1; i < vertCount; ++i) {
        float x0 = data[(i-1)*5 + 0];
        float y0 = data[(i-1)*5 + 1];
        float x1 = data[i*5 + 0];
        float y1 = data[i*5 + 1];
        float dx = x1 - x0;
        float dy = y1 - y0;
        float dist = std::sqrt(dx*dx + dy*dy);

        if(dist > splitThreshold) {
            // draw current segment and start a new one
            drawRawVertices(segment, GL_LINE_STRIP);
            segment.clear();
            // start new segment with this vertex
            pushVert(i);
        }
        else {
            // append vertex to current segment
            pushVert(i);
        }
    }

    if(!segment.empty()) drawRawVertices(segment, GL_LINE_STRIP);
}

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
            // update render target for this marked point
            targetMarkedX[collectedPoints] = distanceX;
            targetMarkedY[collectedPoints] = distanceY;
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
            // update render target for this marked point
            targetMarkedX[collectedPoints] = distanceX;
            targetMarkedY[collectedPoints] = distanceY;
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
            // update render target for this marked point
            targetMarkedX[collectedPoints] = pointInLine[0];
            targetMarkedY[collectedPoints] = pointInLine[1];
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
        // update raw interactive point (used for computations)
        interactivePoint = std::make_tuple(pointInLine[0], pointInLine[1]);
        canDrawInteractivePoint = true;
        // update render target for interactive point (display-only)
        targetInteractiveX = pointInLine[0];
        targetInteractiveY = pointInLine[1];
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

    // initialize smoothing targets/draw positions on first frame
    if(!smoothingInitialized) {
        for(int i=0;i<6;i++) {
            auto [mx, my, _o1, _o2] = markedPoints[i];
            drawMarkedX[i] = targetMarkedX[i] = mx;
            drawMarkedY[i] = targetMarkedY[i] = my;
        }
        {
            auto [ix, iy] = interactivePoint;
            drawInteractiveX = targetInteractiveX = ix;
            drawInteractiveY = targetInteractiveY = iy;
        }
        smoothingInitialized = true;
    }

    // smooth towards targets (simple exponential smoothing)
    for(int i=0;i<6;i++) {
        drawMarkedX[i] += (targetMarkedX[i] - drawMarkedX[i]) * smoothingFactor;
        drawMarkedY[i] += (targetMarkedY[i] - drawMarkedY[i]) * smoothingFactor;
    }
    drawInteractiveX += (targetInteractiveX - drawInteractiveX) * smoothingFactor;

    std::vector<float> buffer; // interleaved x,y,r,g,b

    float x, y, vx, vy;
    Vector3 localCoordPoint;
    Vector3 globalCoordPoint;

    // draw first circle (dark gray)
    for(float i = 0; i < 2 * M_PI; i += 0.001) {
        x = (circleRadius * cos(i)) + offsetCircle1X;
        y = (circleRadius * sin(i)) + offsetCircle1Y;
        buffer.push_back(x);
        buffer.push_back(y);
        buffer.push_back(0.2f);
        buffer.push_back(0.2f);
        buffer.push_back(0.2f);
    }
    drawVertices(buffer, GL_LINE_STRIP);
    buffer.clear();

    // draw line 1 projected onto first circle
    if(collectedPoints >= 2) {
        Vector3 p1, p2;
        getLinePoints(0, p1, p2, circleRadius);

        if(p1[2] < infinityThreshold && p2[2] < infinityThreshold) {
            isIdealLine[0] = true;
        }

        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({p1, p2});
        lineTransformations[0] = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
        lineBaseRotations[0] = std::make_tuple(zRotationAngle, clockwise);

        // Generate main projected arc and opposite-side points (white) for the first circle
        std::vector<float> projBuf;
        std::vector<float> projOppBuf; // opposite-side vertices (draw separately, white)
        for(float i = 0; i < M_PI; i += 0.001) {
            localCoordPoint = Vector3(cos(i), sin(i), 0);
            globalCoordPoint = lineTransformations[0] * localCoordPoint;

            vx = (circleRadius * globalCoordPoint[0]);
            vy = (circleRadius * globalCoordPoint[1]);
            x = vx + offsetCircle1X;
            y = vy + offsetCircle1Y;
            projBuf.push_back(x);
            projBuf.push_back(y);
            projBuf.push_back(1.0f);
            projBuf.push_back(1.0f);
            projBuf.push_back(1.0f);

            if(checkInfinityPoint(vx, vy)) {
                x = -vx + offsetCircle1X;
                y = -vy + offsetCircle1Y;
                projOppBuf.push_back(x);
                projOppBuf.push_back(y);
                projOppBuf.push_back(1.0f);
                projOppBuf.push_back(1.0f);
                projOppBuf.push_back(1.0f);
            }
        }
        drawVertices(projBuf, GL_LINE_STRIP);
        if(!projOppBuf.empty()) drawVertices(projOppBuf, GL_POINTS);
    }

    // draw second circle
    for(float i = 0; i < 2 * M_PI; i += 0.001) {
        x = (circleRadius * cos(i)) + offsetCircle2X;
        y = (circleRadius * sin(i)) + offsetCircle2Y;
        buffer.push_back(x);
        buffer.push_back(y);
        buffer.push_back(0.2f);
        buffer.push_back(0.2f);
        buffer.push_back(0.2f);
    }
    drawVertices(buffer, GL_LINE_STRIP);
    buffer.clear();

    // draw line 2 projected onto second circle
    if(collectedPoints >= 5) {
        Vector3 p1, p2;
        getLinePoints(3, p1, p2, circleRadius);

        if(p1[2] < infinityThreshold && p2[2] < infinityThreshold) {
            isIdealLine[1] = true;
        }

        auto [zRotationAngle, clockwise, xRotationAngle] = calculateRotations({p1, p2});
        lineTransformations[1] = Matrix3::rotationZCos(zRotationAngle, clockwise) * Matrix3::rotationXSin(xRotationAngle);
        lineBaseRotations[1] = std::make_tuple(zRotationAngle, clockwise);

        std::vector<float> projBuf;
        std::vector<float> projOppBuf; // opposite-side vertices (draw separately)
        for(float i = 0; i < M_PI; i += 0.001) {
            localCoordPoint = Vector3(cos(i), sin(i), 0);
            globalCoordPoint = lineTransformations[1] * localCoordPoint;

            vx = (circleRadius * globalCoordPoint[0]);
            vy = (circleRadius * globalCoordPoint[1]);
            x = vx + offsetCircle2X;
            y = vy + offsetCircle2Y;
            projBuf.push_back(x);
            projBuf.push_back(y);
            projBuf.push_back(1.0f);
            projBuf.push_back(1.0f);
            projBuf.push_back(1.0f);

            if(checkInfinityPoint(vx, vy)) {
                x = -vx + offsetCircle2X;
                y = -vy + offsetCircle2Y;
                projOppBuf.push_back(x);
                projOppBuf.push_back(y);
                projOppBuf.push_back(1.0f);
                projOppBuf.push_back(1.0f);
                projOppBuf.push_back(1.0f);
            }
        }
        drawVertices(projBuf, GL_LINE_STRIP);
        // draw opposite-side isolated points so they are not connected to the strip
        if(!projOppBuf.empty()) drawVertices(projOppBuf, GL_POINTS);
    }

    int pointOrder;
    // draw marked points
     for(int j = 0; j < drawablePoints; j++) {
         // restored original per-index channel variation but with a different base palette
         float rgbValues[3] = {0.85f, 0.85f, 0.85f};
         rgbValues[j % 3] = 0.15f; // lower one channel to create distinct color per correspondence
         float redValue = rgbValues[0];
         float greenValue = rgbValues[1];
         float blueValue = rgbValues[2];
         auto[px, py, offsetCircleX, offsetCircleY] = markedPoints[j];
        std::vector<float> ptBuf;
        std::vector<float> ptOppBuf;
        if(checkInfinityPoint(px, py)) {
            for(float i = 0; i < 2 * M_PI; i += 0.001) {
                float pxCircle = (7 * cos(i)) - px + offsetCircleX;
                float pyCircle = (7 * sin(i)) - py + offsetCircleY;
                ptOppBuf.push_back(pxCircle);
                ptOppBuf.push_back(pyCircle);
                ptOppBuf.push_back(redValue);
                ptOppBuf.push_back(greenValue);
                ptOppBuf.push_back(blueValue);
            }
        }
        for(float i = 0; i < 2 * M_PI; i += 0.001) {
            float pxCircle = (7 * cos(i)) + px + offsetCircleX;
            float pyCircle = (7 * sin(i)) + py + offsetCircleY;
            ptBuf.push_back(pxCircle);
            ptBuf.push_back(pyCircle);
            ptBuf.push_back(redValue);
            ptBuf.push_back(greenValue);
            ptBuf.push_back(blueValue);
        }
        drawVertices(ptBuf, GL_LINE_STRIP);
        if(!ptOppBuf.empty()) drawVertices(ptOppBuf, GL_LINE_STRIP);
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

        // draw only the arc (no opposite-side vertices) for pappus support lines
        drawProjectedLine(transform, offsetCircle1X, offsetCircle1Y, circleRadius, false);
        drawProjectedLine(transform, offsetCircle2X, offsetCircle2Y, circleRadius, false);

        //draw interactive point
         if (canDrawInteractivePoint) {
             auto[px, py] = interactivePoint;
             // interactive point circle on first circle (green)
            std::vector<float> itpBuf;
            std::vector<float> itpOppBuf;
            if(checkInfinityPoint(px, py)) {
                for(float i = 0; i < 2 * M_PI; i += 0.001) {
                    float pxCircle = (7 * cos(i)) - px + offsetCircle1X;
                    float pyCircle = (7 * sin(i)) - py + offsetCircle1Y;
                    itpOppBuf.push_back(pxCircle);
                    itpOppBuf.push_back(pyCircle);
                    itpOppBuf.push_back(0.0f); // green R
                    itpOppBuf.push_back(1.0f); // green G
                    itpOppBuf.push_back(0.0f); // green B
                }
            }
            for(float i = 0; i < 2 * M_PI; i += 0.001) {
                float pxCircle = (7 * cos(i)) + px + offsetCircle1X;
                float pyCircle = (7 * sin(i)) + py + offsetCircle1Y;
                itpBuf.push_back(pxCircle);
                itpBuf.push_back(pyCircle);
                itpBuf.push_back(0.0f); // green R
                itpBuf.push_back(1.0f); // green G
                itpBuf.push_back(0.0f); // green B
            }
            drawVertices(itpBuf, GL_LINE_STRIP);
            if(!itpOppBuf.empty()) drawVertices(itpOppBuf, GL_LINE_STRIP);

            // draw the image point and related projected lines
            Vector3 imageLine = y2.cross(y3);
            Vector3 itp = liftToSphere(px, py, circleRadius);
            Vector3 firstCorrrespondenceLine = y1.cross(itp);

            Vector3 pappusIntersection = lineIntersection(pappus, firstCorrrespondenceLine);
            Vector3 secondCorrrespondenceLine = x1.cross(pappusIntersection);

            Vector3 imagePoint = lineIntersection(imageLine, secondCorrrespondenceLine);
            if(imagePoint[2] < 0) imagePoint = imagePoint * -1;
            if(pappusIntersection[2] < 0) pappusIntersection = pappusIntersection * -1;

            // projected line from y1 to itp on circle1
            {
                auto [zR, cw, xR] = calculateRotations({y1, itp});
                Matrix3 tr = Matrix3::rotationZCos(zR, cw) * Matrix3::rotationXSin(xR);
                drawProjectedLine(tr, offsetCircle1X, offsetCircle1Y, circleRadius);
            }

            // projected line for pappusIntersection -> imagePoint on circle2
            {
                auto [zR2, cw2, xR2] = calculateRotations({pappusIntersection, imagePoint});
                Matrix3 tr2 = Matrix3::rotationZCos(zR2, cw2) * Matrix3::rotationXSin(xR2);
                drawProjectedLine(tr2, offsetCircle2X, offsetCircle2Y, circleRadius);
            }

            // draw pappus intersection marker circles on both circles (dark gray)
             {
                 auto [rx, ry, rz] = pappusIntersection;
                std::vector<float> papNegBuf;
                std::vector<float> papPosBuf;
                if(checkInfinityPoint(rx, ry)) {
                    for(float i = 0; i < 2 * M_PI; i += 0.001) {
                        float pxCircle1 = (7 * cos(i)) - rx + offsetCircle1X;
                        float pyCircle1 = (7 * sin(i)) - ry + offsetCircle1Y;
                        float pxCircle2 = (7 * cos(i)) - rx + offsetCircle2X;
                        float pyCircle2 = (7 * sin(i)) - ry + offsetCircle2Y;
                        papNegBuf.push_back(pxCircle1); papNegBuf.push_back(pyCircle1); papNegBuf.push_back(0.1f); papNegBuf.push_back(0.1f); papNegBuf.push_back(0.1f);
                        papNegBuf.push_back(pxCircle2); papNegBuf.push_back(pyCircle2); papNegBuf.push_back(0.1f); papNegBuf.push_back(0.1f); papNegBuf.push_back(0.1f);
                    }
                }
                for(float i = 0; i < 2 * M_PI; i += 0.001) {
                    float pxCircle1 = (7 * cos(i)) + rx + offsetCircle1X;
                    float pyCircle1 = (7 * sin(i)) + ry + offsetCircle1Y;
                    float pxCircle2 = (7 * cos(i)) + rx + offsetCircle2X;
                    float pyCircle2 = (7 * sin(i)) + ry + offsetCircle2Y;
                    papPosBuf.push_back(pxCircle1); papPosBuf.push_back(pyCircle1); papPosBuf.push_back(0.1f); papPosBuf.push_back(0.1f); papPosBuf.push_back(0.1f);
                    papPosBuf.push_back(pxCircle2); papPosBuf.push_back(pyCircle2); papPosBuf.push_back(0.1f); papPosBuf.push_back(0.1f); papPosBuf.push_back(0.1f);
                }
                if(!papPosBuf.empty()) drawVertices(papPosBuf, GL_LINE_STRIP);
                if(!papNegBuf.empty()) drawVertices(papNegBuf, GL_LINE_STRIP);
             }

            // draw image point on second circle (use same orange as interactive point)
             {
                 auto [qx, qy, qz] = imagePoint;
                std::vector<float> imgPosBuf;
                std::vector<float> imgNegBuf;
                if(checkInfinityPoint(qx, qy)) {
                    for(float i = 0; i < 2 * M_PI; i += 0.001) {
                        float pxCircle = (7 * cos(i)) - qx + offsetCircle2X;
                        float pyCircle = (7 * sin(i)) - qy + offsetCircle2Y;
                        // green to match interactive point
                        imgNegBuf.push_back(pxCircle); imgNegBuf.push_back(pyCircle); imgNegBuf.push_back(0.0f); imgNegBuf.push_back(1.0f); imgNegBuf.push_back(0.0f);
                    }
                }
                for(float i = 0; i < 2 * M_PI; i += 0.001) {
                    float pxCircle = (7 * cos(i)) + qx + offsetCircle2X;
                    float pyCircle = (7 * sin(i)) + qy + offsetCircle2Y;
                    imgPosBuf.push_back(pxCircle); imgPosBuf.push_back(pyCircle); imgPosBuf.push_back(0.0f); imgPosBuf.push_back(1.0f); imgPosBuf.push_back(0.0f);
                }
                if(!imgPosBuf.empty()) drawVertices(imgPosBuf, GL_LINE_STRIP);
                if(!imgNegBuf.empty()) drawVertices(imgNegBuf, GL_LINE_STRIP);
             }
        }
    }

    glFlush();
}
