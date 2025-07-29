#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <tuple>
#include "Vector3.h"
#include "Matrix3.h"
#include "utils.h"

// Inicialização do OpenGL
void myInit(void);

// Callbacks do mouse
void mouseClickCallback(int button, int state, int mouseX, int mouseY);
void passiveMouseMotion(int x, int y);

// Função principal de desenho
void display(void);

// Cálculo da rotação em Z
std::tuple<long double, bool> calculateRotationZ(std::tuple<Vector3, Vector3> line);

#endif // GRAPHICS_H
