#include <GL/glew.h>
#include <GL/glut.h>
#include "graphics.h"
#include "utils.h"

int main(int argc,char** argv) {
    glutInit(&argc,argv);
    // Note: removed glutInitContextVersion/glutInitContextProfile for compatibility

    glutInitDisplayMode(GLUT_SINGLE|GLUT_RGB);
    glutInitWindowSize(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);
    glutInitWindowPosition(0,0);
    glutCreateWindow("Pappus Construction - Press F for fullscreen, Q to quit");

    // Initialize GLEW after creating an OpenGL context
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
        return 1;
    }

    myInit();
    initGLResources();
    glutMouseFunc(mouseClickCallback);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutDisplayFunc(display);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutMainLoop();
}

