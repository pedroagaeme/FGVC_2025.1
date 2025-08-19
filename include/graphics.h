#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <tuple>
#include "Vector3.h"
#include "Matrix3.h"
#include "utils.h"

#include <GL/glew.h>
#include <vector>

// Inicialização do OpenGL
void myInit(void);

// initialize shaders/VAO/VBO
extern GLuint shaderProgram;
void initGLResources();

// helper to draw interleaved vertex (x,y,r,g,b) data with a given primitive
void drawVertices(const std::vector<float>& data, GLenum mode);

// Callbacks do mouse
void mouseClickCallback(int button, int state, int mouseX, int mouseY);
void passiveMouseMotion(int x, int y);

// Função principal de desenho
void display(void);

// Cálculo da rotação em Z, sua direção e X
std::tuple<double, bool, double> calculateRotations(std::tuple<Vector3, Vector3> line);

// Desenho de curvas de Bézier
void draw_bezier_curve(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, int offsetX, int offsetY);

#endif // GRAPHICS_H
