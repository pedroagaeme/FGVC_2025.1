#include <GL/glut.h>
#include "graphics.h"
#include "utils.h"

int main(int argc,char** argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_SINGLE|GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
    glutInitWindowPosition(0,0);
    glutCreateWindow("Circle Drawing");

    myInit();
    glutMouseFunc(mouseClickCallback);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutDisplayFunc(display);
    glutMainLoop();
}

