/*
Copyright (c) 2013-2017 Jinrong Xie (jrxie at ucdavis dot edu)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.
*/

//This sample demonstrates how to volume render geodesic grid using the analytic approach
//introduced in the paper "Interactive Ray Casting of Geodesic Grids", doi: 10.1111/cgf.12135
//along with hexagon/triangle mesh visualization with a predefined color map stored in config/tf.vtf
//User is highly recommended to enhance this sample with an interactive transfer function editor
//to adjust the color map on-the-fly.

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
#include <utility.h>
#include <GLContext.h>
#include <GLTexture1D.h>
#include "Dir.h"
#include "GCRMMesh.h"


using namespace std;

int window_width = 800;
int window_height = 600;
int mouseX=0, mouseY=0;
int	mousetype = -1;
float xRotate=0.0f, yRotate=0.0f;
float xTranslate = 0, yTranslate = 0, zTranslate = -5.0f;
float scale = 1.0f;
bool needUpdate = true;

string gridFilePath = "C:/fusion/gcrm_220km/grid.nc";
string dataFilePath = "C:/fusion/gcrm_220km/vorticity_19010115_000000.nc";
string dataVarName  = "vorticity";
string tfFilePath = "./config/tf.vtf";
int tfResolution = 2048;

map<string, map<string, string> > shaderConfig({
    {//shader for rendering dual triangular mesh using the connectivity stored in texture
      "gcrm_triangle_mesh", { { "vertex_shader",   "./shader/gcrm_triangle_mesh.vert" }, 
                              { "geometry_shader", "./shader/gcrm_triangle_mesh.geom" }, 
                              { "fragment_shader", "./shader/gcrm_triangle_mesh.frag" } 
                            }//end of vector list initialization
    },
    {//hexagon mesh 
      "gcrm_hexagon_mesh", { { "vertex_shader",   "./shader/gcrm_hexagon_mesh.vert" }, 
                             { "geometry_shader_frame", "./shader/gcrm_hexagon_mesh_frame.geom" }, 
                             { "geometry_shader_fill", "./shader/gcrm_hexagon_mesh_fill.geom" }, 
                             { "fragment_shader", "./shader/gcrm_hexagon_mesh.frag" } 
                           }//end of vector list initialization
    },
    {//volume ray casting using our proposed method in the paper.
      "gcrm_dvr",          { { "vertex_shader",   "./shader/gcrm_triangle_mesh.vert" }, 
                             { "geometry_shader", "./shader/gcrm_dvr.geom" }, 
                             { "fragment_shader", "./shader/gcrm_dvr.frag" } 
                           }//end of vector list initialization
    }
});

IMeshRef m_mesh;

davinci::mat4 mvm;
davinci::mat4 prjMtx;

//UI toggles
struct UIToggle
{
    bool bDrawVolume = false;
    bool bDrawMesh = true;
    bool bWireframe = true;
    bool bEnableLighting = false;
    float fStepSize = 0.001f;
    string meshType = "triangle";// "hexagon"
}ui;

void display();
void printHelper();

void loadData(){
    if (!m_mesh){
        m_mesh = IMeshRef(new GCRMMesh());
    }
    int timeStart = 0, timeEnd = 1;
    m_mesh->ReadGrid(gridFilePath);
    m_mesh->SetMeshType("hexagon");
    //load data value.
    cout << "Load data " << dataVarName << " from data file" << dataFilePath << endl;

    m_mesh->LoadClimateData(dataFilePath, dataVarName, 0, 0);
    m_mesh->normalizeClimateDataArray(dataVarName);

    m_mesh->Remesh();
    m_mesh->SetDataNew(true);
    m_mesh->InitShaders(shaderConfig);
}

void loadTF(){
    ifstream ifs(tfFilePath, ios::binary);
    if (!ifs){
        cerr << __func__": Cannot find transfer function file " << tfFilePath << endl;
        exit(1);
    }
    vector<unsigned int> tfRGBAArray(tfResolution,0);
    ifs.read(reinterpret_cast<char*>(tfRGBAArray.data()), sizeof(unsigned int)*tfRGBAArray.size());
    ifs.close();
    //convert from BGRA to ABGR(opengl standard)
    for (int i = 0; i < tfResolution; i++)
    {
        tfRGBAArray[i] = davinci::BGRA_to_ABGR(tfRGBAArray[i]);
    }
    //Create 1D transfer function texture(color map)
    davinci::GLTexture1DRef
    tfTex1D = davinci::GLTexture1DRef(new davinci::GLTexture1d(tfResolution,
                    GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, tfRGBAArray.data()));
    tfTex1D->setName("tfTex1D");
    davinci::GLError::glCheckError("loadTF():create transfer function texture failed!");

    m_mesh->glslSetTransferFunc(tfTex1D);
    m_mesh->SetMeshType(ui.meshType);
    davinci::GLError::glCheckError("glslSetTransferFunc(): failed!");
    //initialize one white light default at the origin of camera space.
    davinci::GLContext::g_lights.push_back(davinci::GLLights());
    m_mesh->SetMaterial(davinci::vec4f(0.5f/*Kamb*/, 0.83f/*Kdif*/, 0.89f/*Kspe*/, 20.0f/*Kshi*/));
    m_mesh->SetStepSize(ui.fStepSize);
    m_mesh->ToggleLighting(ui.bEnableLighting);
    m_mesh->SetLight(davinci::GLContext::g_lights[0]);
}

void keyboard(unsigned char key, int x, int y){
    switch(key) 
    {
        case 'w':
            printf("W key pressed!\n");
            zTranslate += 0.1f;
            printf("zTranslate:%f\n",zTranslate);
            break;
        case 's':
            printf("S key pressed!\n");
            zTranslate -= 0.1f;
            printf("zTranslate:%f\n",zTranslate);
            break;
        case 'd':
            printf("d key pressed!\n");
            xTranslate -= 0.1f;
            printf("xTranslate:%f\n",xTranslate);
            break;
        case 'a':
            printf("a key pressed!\n");
            xTranslate += 0.1f;
            printf("xTranslate:%f\n",xTranslate);
            break;
        case ' ':
            printf("space key pressed!\n");
            yTranslate -= 0.1f;
            printf("yTranslate:%f\n",yTranslate);
            break;
        case 'z':
            yTranslate += 0.1f;
            printf("yTranslate:%f\n",yTranslate);
            break;
        case 'v'://toggle btw. volume rendering and mesh rasterization
            printf("v key pressed!\n");
            ui.bDrawVolume = !ui.bDrawVolume;
            ui.bDrawMesh = !ui.bDrawVolume;
            printf("%s volume rendering.\n", (!ui.bDrawVolume? "Enable":"Disable"));
            break;
        case 'h'://toggle btw. hexagon and triangle
            printf("h key pressed!\n");
            ui.meshType = (ui.meshType == "triangle" ? "hexagon" : "triangle");
            printf("mesh type: %s.\n", ui.meshType.data());
            m_mesh->SetMeshType(ui.meshType);
            break;
        case 'f'://toggle btw. wireframe and solid
            printf("f key pressed!\n");
            ui.bWireframe = !ui.bWireframe;
            printf("%s wireframe .\n", (!ui.bWireframe ? "Enable":"Disable"));
            m_mesh->ToggleWireframe(ui.bWireframe);
            break;
        case 'l'://toggle lighting applied to both volume rendering and rasterization
            ui.bEnableLighting = !ui.bEnableLighting;
            printf("%s lighting.\n", (ui.bEnableLighting ? "Enable":"Disable"));
            m_mesh->ToggleLighting(ui.bEnableLighting);
            break;
        case '+'://increase raycasting stepsize
            ui.fStepSize += 0.001f;
            printf("stepsize=%f\n",ui.fStepSize);
            m_mesh->SetStepSize(ui.fStepSize);
            break;
        case '-'://decrease raycasting stepsize
            ui.fStepSize -= 0.001f;
            ui.fStepSize = std::max(0.001f, ui.fStepSize);
            printf("stepsize=%f\n",ui.fStepSize);
            m_mesh->SetStepSize(ui.fStepSize);
            break;
        case(27) :
            printf("ESC key pressed!\n");
            exit(0);
    }
    needUpdate = true;
    display();
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
    if (dir > 0){
        // Zoom out
        scale *= factor;
    }else{
        // Zoom in
        scale /= factor;
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
    loadData();
    loadTF();

    glClearColor(0,0,0,1.0);
    glViewport(0,0,window_width, window_height);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    printHelper();

    needUpdate = true;
    return true;
}

void reshape(int w, int h){
    window_width = w; window_height = h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //gluPerspective(80.0f, (float)w/h, 0.1, 100);
    prjMtx = davinci::mat4::createPerpProjMatrix(80.0f, (float)w / h, 0.1, 100);
    glLoadTransposeMatrixf(prjMtx.get());
    davinci::GLContext::g_PjM = prjMtx;
    needUpdate = true;
}

void renderMesh(){
    davinci::GLContext::g_MVM = mvm;
    davinci::GLContext::g_NM = mvm.getNormalMatrix();
    if (ui.bDrawMesh){
        m_mesh->RenderGrid();
    }
    if (ui.bDrawVolume){
        m_mesh->volumeRender();
    }
}

void display(){
    if (needUpdate){//only refresh when it's necessary.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        mvm.identity();
        mvm.rotateYd(yRotate);
        mvm.rotateXd(xRotate);
        mvm.scale(scale);
        mvm.translate(0, 0, -2.0f);//move object 2 unit away from camera
        mvm.translate(xTranslate, yTranslate, zTranslate);//apply camera motion

        glLoadTransposeMatrixf(mvm.get());

        //glutWireCube(2);//for debug and sanity check of transformation matrix
        renderMesh();

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
    glutCreateWindow("Direct Volume Rendering of Geodesic Grid");
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

void printHelper(){
    cout << " key 'v': toggle between volume rendering and mesh visualization\n"
        << " key 'h': toggle between hexagon and dual-triangular mesh visualization\n"
        << " key 'f': toggle between solid and wire frame mesh\n"
        << " key 'l': toggle between enabling and disabling lighting for both volume render and mesh visualization\n"
        << " key '+': increase raycasting stepsize (higher frame rate)\n"
        << " key '-': decrease raycasting stepsize (better quality)\n"
        << " key 'w': move camera forward\n"
        << " key 's': move camera backward\n"
        << " key 'a': move camera to the left\n"
        << " key 'd': move camera to the right\n"
        << " key ' ': move camera upward\n"
        << " key 'z': move camera downward\n"
        << " mouse dragging: rotate object\n"
        << " mouse wheel: scale object\n";
}
