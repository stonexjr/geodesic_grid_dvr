//This tutorial demonstrates how to quick prototype an OpenGL app using freeglut
// 
#if defined(__APPLE__) || defined(MACOSX)
#include <gl.h>
#include <gl3.h>
#include <OpenGL.h>
#else
#include <GL/glew.h>
#endif
#include <iostream>
#include <GL/freeglut.h>
#include <mat4.h>

using namespace std;

int window_width = 800;
int window_height = 600;
int mouseX=0, mouseY=0;
int	mousetype = -1;
float xRotate=0.0f, yRotate=0.0f;
float xTranslate = 0, yTranslate = 0, zTranslate = -5.0f;
float scale = 1.0f;
bool needUpdate = true;

davinci::mat4 mvm;
davinci::mat4 prjMtx;


void display();

void keyboard(unsigned char key, int x, int y){
    switch(key) 
    {
        case 'w':
            printf("W key pressed!\n");
            break;
        case(27) :
            printf("ESC key pressed!\n");
            exit(0);
    }
}

void mouseMotion(int x , int y){
    if (mousetype == GLUT_LEFT_BUTTON)
    {
        yRotate += (x-mouseX)*0.5f;
        xRotate += (y-mouseY)*0.5f;
        yRotate = int(yRotate)%360;
        xRotate = int(xRotate)%360;
        printf("xRotate=%f, yRotate=%f\n",xRotate,yRotate);
    }else if (mousetype == GLUT_MIDDLE_BUTTON)
    {
        xTranslate += (x-mouseX)*0.005f;
        yTranslate += (mouseY-y)*0.005f;
        printf("xTranslate=%f, yTranslate=%f, zTranslate=%f\n",xTranslate,yTranslate,zTranslate);
    }
    mouseX = x;
    mouseY = y;   
    needUpdate = true;
    display();
}

void mouseButton(int button, int state, int x , int y){
    printf("mouse button(%d) x=%d, y=%d\n",button,x,y);
    if (button == GLUT_LEFT_BUTTON)
    {
        //add response when left mouse button is pressed.
        printf("Left mouse button clicked!\n");
    }else if (button == GLUT_MIDDLE_BUTTON)
    {
        //add response when right mouse button is pressed.
        printf("Middle mouse button clicked!\n");
    }else if (button == GLUT_RIGHT_BUTTON)
    {
        //add response when right mouse button is pressed.
        printf("Right mouse button clicked!\n");
    }

    mouseX = x; mouseY = y;
    mousetype = button;
    needUpdate = true;
    display();
}

void mouseWheel(int button, int dir, int x, int y){
    double factor = 0.9;
    if (dir > 0)
    {
        // Zoom in
        scale /= factor;
    }
    else
    {
        // Zoom out
        scale *= factor;
    }
    cout << "scale=" << scale << endl;

    needUpdate = true;
    display();
}

bool initGL(){
    glewInit();
    if (! glewIsSupported(
        "GL_VERSION_2_0 " 
        "GL_ARB_pixel_buffer_object "
        "GL_EXT_framebuffer_object "
        ))
    {
        fprintf(stderr, "ERROR: Support for necessary OpenGL extensions missing.");
        fflush(stderr);
        return false;
    }
    glClearColor(0,0,0,1.0);

    glViewport(0,0,window_width, window_height);

    needUpdate = true;
    return true;
}

void reshape(int w, int h){
    window_width = w; window_height = h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(80.0f, (float)w/h, 0.1, 100);
    float extent = 3;
    float n = -20, f = 20;
    //glOrtho(-extent, extent, -extent, extent, n, f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
void display(){
    if (needUpdate){//only refresh when it's necessary.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        mvm.identity();
        mvm.rotateYd(yRotate);
        mvm.rotateXd(xRotate);
        mvm.scale(scale);
        mvm.translate(xTranslate, yTranslate, zTranslate);

        glLoadTransposeMatrixf(mvm.get());

        glutWireCube(2);

        glutSwapBuffers();
        glutPostRedisplay();
        needUpdate = false;
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Simple GLUT");
    //glutFullScreen();
    //glutGameModeString("1024x768:32");
    //glutEnterGameMode();

    if (!initGL())
    {
        return 0;
    }
    //register callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutMouseWheelFunc(mouseWheel);

    glutMainLoop();

    return 0;
}
