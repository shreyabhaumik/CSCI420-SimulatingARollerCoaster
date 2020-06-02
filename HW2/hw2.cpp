/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Simulating a Roller Coaster
  C++ starter code

  Student username: Shreya Bhaumik
*/

#include "basicPipelineProgram.h"
#include "texPipelineProgram.h"
#include "objloader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

OpenGLMatrix matrix;
BasicPipelineProgram *pipelineProgram, *OBJPipelineProgram;
TexPipelineProgram *texturePipelineProgram;

// To make the scene rotate automatically about y axis
bool autoRot = false; int toggle = -1;
int tempCamAt, justPressed = 0;

int shotCount = 0;  // Screenshot counter
bool takeShots = false; // To start 1000 screenshots
int camAt = 0;  // control speed of ride
float alpha1 = 0.03f, alpha2 = 0.01f; // for use in computing rail cross-section vertices
float d = 150.0f; // size of skybox

glm::mat4 B;  // basis matrix
glm::mat4x3 C;  // control points

// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;
};

std::vector<glm::vec3> splinePoints;
std::vector<bool> isEndOfSpline; // to keep track if the spline point is an end of track or not
std::vector<glm::vec3> tangents, normals, binormals; // TNB
std::vector<glm::vec3> leftRail, sleeper, rightRail; // rail triangle vertices
std::vector<glm::vec3> leftRail_N, sleeper_N, rightRail_N; // rail tiangle vertex normals
GLuint vboLeftRail, vboLeftRail_N, vboSleeper, vboSleeper_N, vboRightRail, vboRightRail_N; // rail vbos
GLuint vaoLeftRail, vaoSleeper, vaoRightRail; // rail vaos

std::vector<glm::vec3> leftSupport, rightSupport; // rail support triangle vertices
std::vector<glm::vec3> leftSupport_N, rightSupport_N; // rail support tiangle vertex normals
GLuint vboLeftSupport, vboLeftSupport_N, vboRightSupport, vboRightSupport_N; // rail support vbos
GLuint vaoLeftSupport, vaoRightSupport; // rail support vaos

std::vector<glm::vec3> skyboxLPos, skyboxRPos, skyboxFPos, skyboxBPos, skyboxDPos, skyboxTPos;
std::vector<glm::vec2> skyboxLUV, skyboxRUV, skyboxFUV, skyboxBUV, skyboxDUV, skyboxTUV;
GLuint vboSkyboxL, vboSkyboxR, vboSkyboxF, vboSkyboxB, vboSkyboxD, vboSkyboxT; 
GLuint vaoSkyboxL, vaoSkyboxR, vaoSkyboxF, vaoSkyboxB, vaoSkyboxD, vaoSkyboxT;
GLuint texHandleL, texHandleR, texHandleF, texHandleB, texHandleD, texHandleT;

// To load object - STANFORD DRAGON
std::vector<glm::vec3> OBJvertices;
std::vector<glm::vec3> OBJnormals;
OBJloader myOBJ;
GLuint vboOBJvertices, vboOBJnormals, vaoOBJ;

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;


// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  int ww = windowWidth * 2;
  int hh = windowHeight * 2;
  unsigned char * screenshotData = new unsigned char[ww * hh * 3 * 4];
  glReadPixels(0, 0, ww, hh, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(ww, hh, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void setTextureUnit(GLint unit)
{
  glActiveTexture(unit);  // select the active texture unit
  // get a handle to the "textureImage" shader variable
  GLint h_textureImage = glGetUniformLocation(texturePipelineProgram->GetProgramHandle(), "textureImage");
  // deem the shader variable "textureImage" to read from texture unit "unit"
  glUniform1i(h_textureImage, unit - GL_TEXTURE0);
}

void passPhongShadingForSkybox() // passing values to shaders for texturing skybox
{
  // For texture - skybox
  float La[4] = {0.45f, 0.15f, 0.15f}; float ka[4] = {0.8f, 0.8f, 0.8f};
  GLint h_La = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "La");
  glUniform4fv(h_La, 1, La);
  GLint h_ka = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ka");
  glUniform4fv(h_ka, 1, ka);

  float Ld[4] = {0.7f, 0.2f, 0.1f}; float kd[4] = {0.9f, 0.9f, 0.9f};
  GLint h_Ld = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ld");
  glUniform4fv(h_Ld, 1, Ld);
  GLint h_kd = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "kd");
  glUniform4fv(h_kd, 1, kd);

  float Ls[4] = {0.75f, 0.15f, 0.0f}; float ks[4] = {1.0f, 1.0f, 1.0f};
  GLint h_Ls = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ls");
  glUniform4fv(h_Ls, 1, Ls);
  GLint h_ks = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ks");
  glUniform4fv(h_ks, 1, ks);

  GLint h_alpha = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "alpha");
  glUniform1f(h_alpha, 1.0f);

  float view[16]; // column-major
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.GetMatrix(view);

  glm::vec3 lightDirection = glm::vec3(0.0f, 1.0f, 0.0f); // the "Sun" at noon
  float viewLightDirection[3]; // light direction in the view space
  // viewLightDirection = (view * float4(lightDirection, 0.0)).xyz;
  viewLightDirection[0] = (view[0] * lightDirection.x) + (view[4] * lightDirection.y) + (view[8] * lightDirection.z);
  viewLightDirection[1] = (view[1] * lightDirection.x) + (view[5] * lightDirection.y) + (view[9] * lightDirection.z);
  viewLightDirection[2] = (view[2] * lightDirection.x) + (view[6] * lightDirection.y) + (view[10] * lightDirection.z);
  //upload viewLightDirection to the GPU
  GLint h_viewLightDirection = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "viewLightDirection");
  glUniform3fv(h_viewLightDirection, 1, viewLightDirection);

  GLint h_normalMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "normalMatrix");
  float n[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(n); // get normal matrix
  glUniformMatrix4fv(h_normalMatrix, 1, GL_FALSE, n);
}

void passPhongShadingForOBJ() // passing values for shaders for rendering STANFORD DRAGON
{
  // For object - dragon (kindof-golden/yellow metallic look)
  float LaOBJ[4] = {0.5f, 0.5f, 0.5f}; float kaOBJ[4] = {0.5f, 0.2f, 0.0f};
  GLint h_LaOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "La");
  glUniform4fv(h_LaOBJ, 1, LaOBJ);
  GLint h_kaOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "ka");
  glUniform4fv(h_kaOBJ, 1, kaOBJ);

  float LdOBJ[4] = {0.7f, 0.7f, 0.7f}; float kdOBJ[4] = {0.7f, 0.7f, 0.1f};
  GLint h_LdOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "Ld");
  glUniform4fv(h_LdOBJ, 1, LdOBJ);
  GLint h_kdOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "kd");
  glUniform4fv(h_kdOBJ, 1, kdOBJ);

  float LsOBJ[4] = {0.9f, 0.9f, 0.9f}; float ksOBJ[4] = {0.9f, 0.6f, 0.0f};
  GLint h_LsOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "Ls");
  glUniform4fv(h_LsOBJ, 1, LsOBJ);
  GLint h_ksOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "ks");
  glUniform4fv(h_ksOBJ, 1, ksOBJ);

  GLint h_alphaOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "alpha");
  glUniform1f(h_alphaOBJ, 1.0f);

  float viewOBJ[16]; // column-major
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.GetMatrix(viewOBJ);

  glm::vec3 lightDirectionOBJ = glm::vec3(0.0f, 1.0f, 0.0f); // the "Sun" at noon
  float viewLightDirectionOBJ[3]; // light direction in the view space
  // viewLightDirection = (view * float4(lightDirection, 0.0)).xyz;
  viewLightDirectionOBJ[0] = (viewOBJ[0] * lightDirectionOBJ.x) + (viewOBJ[4] * lightDirectionOBJ.y) + (viewOBJ[8] * lightDirectionOBJ.z);
  viewLightDirectionOBJ[1] = (viewOBJ[1] * lightDirectionOBJ.x) + (viewOBJ[5] * lightDirectionOBJ.y) + (viewOBJ[9] * lightDirectionOBJ.z);
  viewLightDirectionOBJ[2] = (viewOBJ[2] * lightDirectionOBJ.x) + (viewOBJ[6] * lightDirectionOBJ.y) + (viewOBJ[10] * lightDirectionOBJ.z);
  //upload viewLightDirection to the GPU
  GLint h_viewLightDirectionOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "viewLightDirection");
  glUniform3fv(h_viewLightDirectionOBJ, 1, viewLightDirectionOBJ);

  GLint h_normalMatrixOBJ = glGetUniformLocation(OBJPipelineProgram->GetProgramHandle(), "normalMatrix");
  float nOBJ[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(nOBJ); // get normal matrix
  glUniformMatrix4fv(h_normalMatrixOBJ, 1, GL_FALSE, nOBJ);
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  // matrix.LookAt(1, 0, 2, 0, 0, 0, 0, 1, 0);
  if(!autoRot) // if rotate is toggled on then don't ride on track
  {
    glm::vec3 eyePos = splinePoints[camAt] +  binormals[camAt] * 0.5f;
    glm::vec3 focus = eyePos + tangents[camAt];
    glm::vec3 up = binormals[camAt];
    matrix.LookAt(eyePos.x, eyePos.y, eyePos.z, focus.x, focus.y, focus.z, up.x, up.y, up.z);
  }

  // For transformations
  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix.Rotate(landRotate[0], 1.0, 0.0, 0.0);
  matrix.Rotate(landRotate[1], 0.0, 1.0, 0.0);
  matrix.Rotate(landRotate[2], 0.0, 0.0, 1.0);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);

  // ModelView matrix - column major
  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);

  // Projection matrix - column major
  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);
  
  // bind shader
  pipelineProgram->Bind();
  // set variable
  // To upload MV matrix to GPU
  pipelineProgram->SetModelViewMatrix(m);
  // As also done by
  // GLuint modelViewMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "modelViewMatrix");
  // glUniformMatrix4fv(modelViewMatrix, 1, GL_FALSE, m);
  // To upload P matrix to GPU
  pipelineProgram->SetProjectionMatrix(p);
  // As also done by
  // GLuint projectionMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "projectionMatrix");
  // glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, p);

  texturePipelineProgram->Bind();
  texturePipelineProgram->SetModelViewMatrix(m);
  texturePipelineProgram->SetProjectionMatrix(p);

  float mOBJ[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.Translate(0.0f, 25.0f, -21.0f);
  matrix.Rotate(180, 1.0, 0.0, 0.0);
  matrix.Rotate(180, 0.0, 1.0, 0.0);
  matrix.Scale(4.0f,4.0f,4.0f);
  matrix.GetMatrix(mOBJ);
  float pOBJ[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(pOBJ);

  OBJPipelineProgram->Bind();
  OBJPipelineProgram->SetModelViewMatrix(mOBJ);
  OBJPipelineProgram->SetProjectionMatrix(pOBJ);

  pipelineProgram->Bind();
  passPhongShadingForSkybox();
  glBindVertexArray(vaoLeftRail);
  glDrawArrays(GL_TRIANGLES, 0, leftRail.size());
  glBindVertexArray(vaoSleeper);
  glDrawArrays(GL_TRIANGLES, 0, sleeper.size());
  glBindVertexArray(vaoRightRail);
  glDrawArrays(GL_TRIANGLES, 0, rightRail.size());
  glBindVertexArray(vaoLeftSupport);
  glDrawArrays(GL_TRIANGLES, 0, leftSupport.size());
  glBindVertexArray(vaoRightSupport);
  glDrawArrays(GL_TRIANGLES, 0, rightSupport.size());

  // glBindVertexArray(vaoSpline);  // FOR MILESTONE
  // glDrawArrays(GL_LINE_STRIP, 0, splinePoints.size());
  // glDrawArrays(GL_LINES, 0, splinePoints.size());  // FOR MILESTONE

  // glBindVertexArray(triVertexArray);
  // glDrawArrays(GL_TRIANGLES, 0, sizeTri);

  // setTextureUnit(GL_TEXTURE0);
  // glBindTexture(GL_TEXTURE_2D, texHandle);

  OBJPipelineProgram->Bind();
  passPhongShadingForOBJ();
  glBindVertexArray(vaoOBJ);
  glDrawArrays(GL_TRIANGLES, 0, OBJvertices.size());

  texturePipelineProgram->Bind();

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleL);
  glBindVertexArray(vaoSkyboxL);
  glDrawArrays(GL_TRIANGLES, 0, skyboxLPos.size());

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleR);
  glBindVertexArray(vaoSkyboxR);
  glDrawArrays(GL_TRIANGLES, 0, skyboxRPos.size());

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleF);
  glBindVertexArray(vaoSkyboxF);
  glDrawArrays(GL_TRIANGLES, 0, skyboxFPos.size());

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleB);
  glBindVertexArray(vaoSkyboxB);
  glDrawArrays(GL_TRIANGLES, 0, skyboxBPos.size());

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleD);
  glBindVertexArray(vaoSkyboxD);
  glDrawArrays(GL_TRIANGLES, 0, skyboxDPos.size());

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleT);
  glBindVertexArray(vaoSkyboxT);
  glDrawArrays(GL_TRIANGLES, 0, skyboxTPos.size());


  glBindVertexArray(0);
  glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff... 
  // controlling the speed of ride on roller coaster
  if(camAt < splinePoints.size())
    camAt += 1;
  else
    camAt = 0;

  // To make the scene rotate
  if ((toggle == -1) && (justPressed == 1))
  {
    autoRot = false;
    camAt = tempCamAt;
    landTranslate[1] = 0.0f;
    landRotate[1] = 0.0f;
    landRotate[2] = 0.0f;
    justPressed = 0;
  }
  if (autoRot)
  {
    if(justPressed == 1)
    {
      justPressed = 0;
      tempCamAt = camAt;
      camAt = 0;
    }
    matrix.LookAt(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    landTranslate[1] = 25.0f;
    landRotate[1] += 0.7f;
    landRotate[2] = 180.0f;
  }

  // To save the 1000 screenshots to disk
  if ((((shotCount) >= 0) && ((shotCount) < 1000)) && (takeShots))
  {
    char filenum[4];
    sprintf(filenum, "%03d", (shotCount));
    std::string filename("Animations/" + std::string(filenum) + ".jpg");
    saveScreenshot(filename.c_str());
    shotCount++;
  }
  else if ((shotCount) >= 1000)
    takeShots = false;

  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 100.0f);
  matrix.Perspective(60.0f, (float)w / (float)h, 0.01f, 2000.0f); // Since 60 is the FOV of human eye
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton && autoRot)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton && autoRot)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton && autoRot)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton && autoRot)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    // CTRL is not working on macOS Catalina 10.15.3
    // case GLUT_ACTIVE_CTRL:
    //   controlState = TRANSLATE;
    // break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  GLint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;

    case 't': // CTRL is not working on macOS Catalina 10.15.3 so we use t for translate
      controlState = TRANSLATE;
    break;

    case 'r': 
      /* To make the scene rotate automatically about y axis
         Toggle to get back to camera riding roller coaster at the point it last left
      */
      autoRot = true;
      justPressed = 1;
      toggle = -toggle;
      landRotate[0] = landRotate[1] = landRotate[2] = 0.0f;
      landTranslate[0] = landTranslate[1] = landTranslate[2] = 0.0f;
    break;

    case 's': // To take 300 screenshots
      takeShots = true;
    break;
  }
}

int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file\n");
    exit(1);
  }
  
  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
     &splines[j].points[i].x, 
     &splines[j].points[i].y, 
     &splines[j].points[i].z) != EOF) 
    {
      i++;
    }
  }

  free(cName);

  return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // added for skybox
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // added for skybox
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;

  return 0;
}

// To make the spline points, tangents, normals and binormals
void getPointsAndTNB()
{
  glm::mat3x4 CTtemp;
  double s = 0.5;
  double basis[16] = {-s, 2.0f-s, s-2.0f, s,
                      2.0f*s, s-3.0f, 3.0f-2.0f*s, -s, 
                      -s, 0.0f, s, 0.0f,
                      0.0f, 1.0f, 0.0f, 0.0f};
  B = glm::make_mat4x4(basis);
  glm::mat4 BT = transpose(B);

  int k = 0;
  for (int i=0; i < numSplines; i++)
  {
    for (int j=0; j < splines[i].numControlPoints - 3; j++)
    {
      double controlpts[12] = {splines[i].points[j].x, splines[i].points[j].y, splines[i].points[j].z,
                              splines[i].points[j+1].x, splines[i].points[j+1].y, splines[i].points[j+1].z,
                              splines[i].points[j+2].x, splines[i].points[j+2].y, splines[i].points[j+2].z,
                              splines[i].points[j+3].x, splines[i].points[j+3].y, splines[i].points[j+3].z};

      C = glm::make_mat4x3(controlpts);
      glm::mat3x4 CT = transpose(C);
      // WAS FOR MILESTONE
      // for (float u = 0.000f; u <= 1.000f; u += 0.002f)
      // {
      //   glm::vec4 U = glm::vec4(pow(u,3), pow(u,2), u, 1);
        
      //   glm::vec3 splinePoint = U * BT * CT;
      //   if ((u == 0.000f) && (j == 0))  // For first point of the entire spline
      //     splinePoints.push_back(splinePoint);
      //   else if ((u + 0.002f > 1.000f) && (j == splines[i].numControlPoints - 4)) // For last point of the entire spline (since u is float, could not do direct comparison with 1.000f as it was not being an exact match ever.)
      //     splinePoints.push_back(splinePoint);
      //   else
      //   {
      //     splinePoints.push_back(splinePoint);
      //     splinePoints.push_back(splinePoint);
      //   }
      // }
      for (float u = 0.000f; u <= 1.000f; u += 0.01f)
      {
        glm::vec4 U = glm::vec4(pow(u,3), pow(u,2), u, 1);
        glm::vec3 splinePoint = U * BT * CT;
        splinePoints.push_back(splinePoint);

        if (j == splines[i].numControlPoints - 4)
          isEndOfSpline.push_back(true);
        else
          isEndOfSpline.push_back(false);

        glm::vec4 dU = glm::vec4(3*pow(u,2), 2*u, 1, 0);
        glm::vec3 tangent = dU * BT * CT;
        tangents.push_back(glm::normalize(tangent));
      }
    }
    //  For 1st spline point       : N0 = unit(T0 x V) and B0 = unit(T0 x N0)
    //  For all other spline points: N1 = unit(B0 x T1) and B1 = unit(T1 x N1).
    glm::vec3 v = glm::vec3(0.0f, 1.0f, 0.0f);
    if (tangents[k] == v)
      v = glm::vec3(0.0f, 0.0f, 1.0f); // v = glm::vec3(0.9f, 0.0f, 0.43f);
    glm::vec3 normal = glm::normalize(glm::cross(tangents[k], v));
    normals.push_back(normal);
    glm::vec3 binormal = glm::normalize(glm::cross(tangents[k], normal));
    binormals.push_back(binormal);
    k++;
    for (; k < splinePoints.size(); k++)
    {
      glm::vec3 normal = glm::normalize(glm::cross(binormals[k-1], tangents[k]));
      normals.push_back(normal);
      glm::vec3 binormal = glm::normalize(glm::cross(tangents[k], normals[k]));
      binormals.push_back(binormal);
    }
  }
}

glm::vec3 getRailVertex(glm::vec3 p, glm::vec3 n, glm::vec3 b)
{
  glm::vec3 v;
  // float alpha1 = 0.03f, alpha2 = 0.01f; - put them as global
  n = n * alpha1;
  b = b * alpha2;
  v = p + n + b;
  return v;
}

glm::vec3 getLowerTVertex(glm::vec3 p, glm::vec3 n, glm::vec3 b)
{
  glm::vec3 v;
  // float alpha1 = 0.03f, alpha2 = 0.01f; - put them as global
  n = n * alpha2;
  b = b * alpha1;
  v = p + n + b;
  return v;
}

// To make hollow rail tracks
/* My rails are one above the other. The right rail is actually the one above, and the left rail is below.
   The sleeper therefore runs between the top face of left rail and botton face of right rail.
*/
void buildRailsAndSupports()
{
  /* The formula is:
  v0 = p0 + alpha * (-n0 + b0)
  v1 = p0 + alpha * (n0 + b0)
  v2 = p0 + alpha * (n0 - b0)
  v3 = p0 + alpha * (-n0 - b0)
  But I want the track's cross section to be rectangular, so I'll use:
  v0 = p0 + (alpha1 * -n0) + (alpha2 * b0)
  v1 = p0 + (alpha1 * n0) + (alpha2 * b0)
  v2 = p0 + (alpha1 * n0) + (alpha2 * -b0)
  v3 = p0 + (alpha1 * -n0) + (alpha2 * -b0)
  */
  glm::vec3 leftRail_p0, rightRail_p0, leftRail_p1, rightRail_p1;
  glm::vec3 leftRail_v0, leftRail_v1, leftRail_v2, leftRail_v3, leftRail_v4, leftRail_v5, leftRail_v6, leftRail_v7;
  glm::vec3 rightRail_v0, rightRail_v1, rightRail_v2, rightRail_v3, rightRail_v4, rightRail_v5, rightRail_v6, rightRail_v7;

  glm::vec3 leftLowerT_p0, rightLowerT_p0, leftLowerT_p1, rightLowerT_p1;
  glm::vec3 leftLowerT_v0, leftLowerT_v1, leftLowerT_v2, leftLowerT_v3, leftLowerT_v4, leftLowerT_v5, leftLowerT_v6, leftLowerT_v7;
  glm::vec3 rightLowerT_v0, rightLowerT_v1, rightLowerT_v2, rightLowerT_v3, rightLowerT_v4, rightLowerT_v5, rightLowerT_v6, rightLowerT_v7;

  glm::vec3 leftSupport_v0, leftSupport_v1, leftSupport_v2, leftSupport_v3, leftSupport_v4, leftSupport_v5, leftSupport_v6, leftSupport_v7;
  glm::vec3 rightSupport_v0, rightSupport_v1, rightSupport_v2, rightSupport_v3, rightSupport_v4, rightSupport_v5, rightSupport_v6, rightSupport_v7;

  for (int i=0; i < splinePoints.size() - 1; i++)
  {
    if (isEndOfSpline[i])
      continue;
    // Left rail vertices                                                           // Right rail vertices
    leftRail_p0 = splinePoints[i] - 0.12f*normals[i];                               rightRail_p0 = splinePoints[i] + 0.12f*normals[i];
    leftRail_v0 = getRailVertex(leftRail_p0, -1.0f*normals[i], binormals[i]);       rightRail_v0 = getRailVertex(rightRail_p0, -1.0f*normals[i], binormals[i]);
    leftRail_v1 = getRailVertex(leftRail_p0, normals[i], binormals[i]);             rightRail_v1 = getRailVertex(rightRail_p0, normals[i], binormals[i]);
    leftRail_v2 = getRailVertex(leftRail_p0, normals[i], -1.0f*binormals[i]);       rightRail_v2 = getRailVertex(rightRail_p0, normals[i], -1.0f*binormals[i]);
    leftRail_v3 = getRailVertex(leftRail_p0, -1.0f*normals[i], -1.0f*binormals[i]); rightRail_v3 = getRailVertex(rightRail_p0, -1.0f*normals[i], -1.0f*binormals[i]);

    int j=i+1;
    leftRail_p1 = splinePoints[j] - 0.12f*normals[j];                               rightRail_p1 = splinePoints[j] + 0.12f*normals[j];
    leftRail_v4 = getRailVertex(leftRail_p1, -1.0f*normals[j], binormals[j]);       rightRail_v4 = getRailVertex(rightRail_p1, -1.0f*normals[j], binormals[j]);
    leftRail_v5 = getRailVertex(leftRail_p1, normals[j], binormals[j]);             rightRail_v5 = getRailVertex(rightRail_p1, normals[j], binormals[j]);
    leftRail_v6 = getRailVertex(leftRail_p1, normals[j], -1.0f*binormals[j]);       rightRail_v6 = getRailVertex(rightRail_p1, normals[j], -1.0f*binormals[j]);
    leftRail_v7 = getRailVertex(leftRail_p1, -1.0f*normals[j], -1.0f*binormals[j]); rightRail_v7 = getRailVertex(rightRail_p1, -1.0f*normals[j], -1.0f*binormals[j]);

    // For lowerT
    leftLowerT_p0 = leftRail_p0 - 0.02f*binormals[i];                               rightLowerT_p0 = rightRail_p0 - 0.02f*binormals[i];
    leftLowerT_v0 = getLowerTVertex(leftLowerT_p0, -1.0f*normals[i], binormals[i]);       rightLowerT_v0 = getLowerTVertex(rightLowerT_p0, -1.0f*normals[i], binormals[i]);
    leftLowerT_v1 = getLowerTVertex(leftLowerT_p0, normals[i], binormals[i]);             rightLowerT_v1 = getLowerTVertex(rightLowerT_p0, normals[i], binormals[i]);
    leftLowerT_v2 = getLowerTVertex(leftLowerT_p0, normals[i], -1.0f*binormals[i]);       rightLowerT_v2 = getLowerTVertex(rightLowerT_p0, normals[i], -1.0f*binormals[i]);
    leftLowerT_v3 = getLowerTVertex(leftLowerT_p0, -1.0f*normals[i], -1.0f*binormals[i]); rightLowerT_v3 = getLowerTVertex(rightLowerT_p0, -1.0f*normals[i], -1.0f*binormals[i]);

    leftLowerT_p1 = leftRail_p1 - 0.02f*binormals[j];                              rightLowerT_p1 = rightRail_p1 - 0.02f*binormals[j];
    leftLowerT_v4 = getLowerTVertex(leftLowerT_p1, -1.0f*normals[j], binormals[j]);       rightLowerT_v4 = getLowerTVertex(rightLowerT_p1, -1.0f*normals[j], binormals[j]);
    leftLowerT_v5 = getLowerTVertex(leftLowerT_p1, normals[j], binormals[j]);             rightLowerT_v5 = getLowerTVertex(rightLowerT_p1, normals[j], binormals[j]);
    leftLowerT_v6 = getLowerTVertex(leftLowerT_p1, normals[j], -1.0f*binormals[j]);       rightLowerT_v6 = getLowerTVertex(rightLowerT_p1, normals[j], -1.0f*binormals[j]);
    leftLowerT_v7 = getLowerTVertex(leftLowerT_p1, -1.0f*normals[j], -1.0f*binormals[j]); rightLowerT_v7 = getLowerTVertex(rightLowerT_p1, -1.0f*normals[j], -1.0f*binormals[j]);


    // For the left rail
    // left face
    // triangle 1
    leftRail.push_back(leftRail_v2);  leftRail_N.push_back(-1.0f*binormals[i]);
    leftRail.push_back(leftRail_v6);  leftRail_N.push_back(-1.0f*binormals[j]);
    leftRail.push_back(leftRail_v3);  leftRail_N.push_back(-1.0f*binormals[i]);
    // triangle 2
    leftRail.push_back(leftRail_v3);  leftRail_N.push_back(-1.0f*binormals[i]);
    leftRail.push_back(leftRail_v6);  leftRail_N.push_back(-1.0f*binormals[j]);
    leftRail.push_back(leftRail_v7);  leftRail_N.push_back(-1.0f*binormals[j]);
    // right face
    // triangle 1
    leftRail.push_back(leftRail_v0);  leftRail_N.push_back(binormals[i]);
    leftRail.push_back(leftRail_v4);  leftRail_N.push_back(binormals[j]);
    leftRail.push_back(leftRail_v5);  leftRail_N.push_back(binormals[j]);
    // triangle 2
    leftRail.push_back(leftRail_v0);  leftRail_N.push_back(binormals[i]);
    leftRail.push_back(leftRail_v5);  leftRail_N.push_back(binormals[j]);
    leftRail.push_back(leftRail_v1);  leftRail_N.push_back(binormals[i]);
    // top face
    // triangle 1
    leftRail.push_back(leftRail_v1);  leftRail_N.push_back(normals[i]);
    leftRail.push_back(leftRail_v5);  leftRail_N.push_back(normals[j]);
    leftRail.push_back(leftRail_v2);  leftRail_N.push_back(normals[i]);
    // triangle 2
    leftRail.push_back(leftRail_v2);  leftRail_N.push_back(normals[i]);
    leftRail.push_back(leftRail_v5);  leftRail_N.push_back(normals[j]);
    leftRail.push_back(leftRail_v6);  leftRail_N.push_back(normals[j]);
    // bottom face
    // triangle 1
    leftRail.push_back(leftRail_v3);  leftRail_N.push_back(-1.0f*normals[i]);
    leftRail.push_back(leftRail_v7);  leftRail_N.push_back(-1.0f*normals[j]);
    leftRail.push_back(leftRail_v4);  leftRail_N.push_back(-1.0f*normals[j]);
    // triangle 2
    leftRail.push_back(leftRail_v3);  leftRail_N.push_back(-1.0f*normals[i]);
    leftRail.push_back(leftRail_v4);  leftRail_N.push_back(-1.0f*normals[j]);
    leftRail.push_back(leftRail_v0);  leftRail_N.push_back(-1.0f*normals[i]);
    // For left lowerT
    // left face
    // triangle 1
    leftRail.push_back(leftLowerT_v2);  leftRail_N.push_back(-1.0f*binormals[i]);
    leftRail.push_back(leftLowerT_v6);  leftRail_N.push_back(-1.0f*binormals[j]);
    leftRail.push_back(leftLowerT_v3);  leftRail_N.push_back(-1.0f*binormals[i]);
    // triangle 2
    leftRail.push_back(leftLowerT_v3);  leftRail_N.push_back(-1.0f*binormals[i]);
    leftRail.push_back(leftLowerT_v6);  leftRail_N.push_back(-1.0f*binormals[j]);
    leftRail.push_back(leftLowerT_v7);  leftRail_N.push_back(-1.0f*binormals[j]);
    // right face
    // triangle 1
    leftRail.push_back(leftLowerT_v0);  leftRail_N.push_back(binormals[i]);
    leftRail.push_back(leftLowerT_v4);  leftRail_N.push_back(binormals[j]);
    leftRail.push_back(leftLowerT_v5);  leftRail_N.push_back(binormals[j]);
    // triangle 2
    leftRail.push_back(leftLowerT_v0);  leftRail_N.push_back(binormals[i]);
    leftRail.push_back(leftLowerT_v5);  leftRail_N.push_back(binormals[j]);
    leftRail.push_back(leftLowerT_v1);  leftRail_N.push_back(binormals[i]);
    // top face
    // triangle 1
    leftRail.push_back(leftLowerT_v1);  leftRail_N.push_back(normals[i]);
    leftRail.push_back(leftLowerT_v5);  leftRail_N.push_back(normals[j]);
    leftRail.push_back(leftLowerT_v2);  leftRail_N.push_back(normals[i]);
    // triangle 2
    leftRail.push_back(leftLowerT_v2);  leftRail_N.push_back(normals[i]);
    leftRail.push_back(leftLowerT_v5);  leftRail_N.push_back(normals[j]);
    leftRail.push_back(leftLowerT_v6);  leftRail_N.push_back(normals[j]);
    // bottom face
    // triangle 1
    leftRail.push_back(leftLowerT_v3);  leftRail_N.push_back(-1.0f*normals[i]);
    leftRail.push_back(leftLowerT_v7);  leftRail_N.push_back(-1.0f*normals[j]);
    leftRail.push_back(leftLowerT_v4);  leftRail_N.push_back(-1.0f*normals[j]);
    // triangle 2
    leftRail.push_back(leftLowerT_v3);  leftRail_N.push_back(-1.0f*normals[i]);
    leftRail.push_back(leftLowerT_v4);  leftRail_N.push_back(-1.0f*normals[j]);
    leftRail.push_back(leftLowerT_v0);  leftRail_N.push_back(-1.0f*normals[i]);

    // For sleeper
    if (i % 5 == 0)
    {
      // left face
      // triangle 1
      sleeper.push_back(leftRail_v2);  sleeper_N.push_back(-1.0f*binormals[i]);
      sleeper.push_back(rightRail_v3);  sleeper_N.push_back(-1.0f*binormals[i]);
      sleeper.push_back(rightRail_v7);  sleeper_N.push_back(-1.0f*binormals[j]);
      // triangle 2
      sleeper.push_back(leftRail_v2);  sleeper_N.push_back(-1.0f*binormals[i]);
      sleeper.push_back(rightRail_v7);  sleeper_N.push_back(-1.0f*binormals[j]);
      sleeper.push_back(leftRail_v6);  sleeper_N.push_back(-1.0f*binormals[j]);
      // right face
      // triangle 1
      sleeper.push_back(leftRail_v1);  sleeper_N.push_back(binormals[i]);
      sleeper.push_back(leftRail_v5);  sleeper_N.push_back(binormals[j]);
      sleeper.push_back(rightRail_v4);  sleeper_N.push_back(binormals[j]);
      // triangle 2
      sleeper.push_back(leftRail_v1);  sleeper_N.push_back(binormals[i]);
      sleeper.push_back(rightRail_v4);  sleeper_N.push_back(binormals[j]);
      sleeper.push_back(rightRail_v0);  sleeper_N.push_back(binormals[i]);
      // front face
      // triangle 1
      sleeper.push_back(leftRail_v1);
      sleeper.push_back(rightRail_v0);
      sleeper.push_back(rightRail_v3);
      // triangle 2
      sleeper.push_back(leftRail_v1);
      sleeper.push_back(rightRail_v3);
      sleeper.push_back(leftRail_v2);
      for (int k=0; k<6; k++) sleeper_N.push_back(-1.0f*tangents[i]);
      // back face
      // triangle 1
      sleeper.push_back(leftRail_v5);
      sleeper.push_back(leftRail_v6);
      sleeper.push_back(rightRail_v7);
      // triangle 2
      sleeper.push_back(leftRail_v5);
      sleeper.push_back(rightRail_v7);
      sleeper.push_back(rightRail_v4);
      for (int k=0; k<6; k++) sleeper_N.push_back(tangents[j]);
      // top face
      // triangle 1
      sleeper.push_back(rightRail_v3);  sleeper_N.push_back(normals[i]);
      sleeper.push_back(rightRail_v0);  sleeper_N.push_back(normals[i]);
      sleeper.push_back(rightRail_v4);  sleeper_N.push_back(normals[j]);
      // triangle 2
      sleeper.push_back(rightRail_v3);  sleeper_N.push_back(normals[i]);
      sleeper.push_back(rightRail_v4);  sleeper_N.push_back(normals[j]);
      sleeper.push_back(rightRail_v7);  sleeper_N.push_back(normals[j]);
      // bottom face
      // triangle 1
      sleeper.push_back(leftRail_v2);  sleeper_N.push_back(-1.0f*normals[i]);
      sleeper.push_back(leftRail_v6);  sleeper_N.push_back(-1.0f*normals[j]);
      sleeper.push_back(leftRail_v5);  sleeper_N.push_back(-1.0f*normals[j]);
      // triangle 2
      sleeper.push_back(leftRail_v2);  sleeper_N.push_back(-1.0f*normals[i]);
      sleeper.push_back(leftRail_v5);  sleeper_N.push_back(-1.0f*normals[j]);
      sleeper.push_back(leftRail_v1);  sleeper_N.push_back(-1.0f*normals[i]);
    }

    // For the right rail
    // left face
    // triangle 1
    rightRail.push_back(rightRail_v2);  rightRail_N.push_back(-1.0f*binormals[i]);
    rightRail.push_back(rightRail_v6);  rightRail_N.push_back(-1.0f*binormals[j]);
    rightRail.push_back(rightRail_v3);  rightRail_N.push_back(-1.0f*binormals[i]);
    // triangle 2
    rightRail.push_back(rightRail_v3);  rightRail_N.push_back(-1.0f*binormals[i]);
    rightRail.push_back(rightRail_v6);  rightRail_N.push_back(-1.0f*binormals[j]);
    rightRail.push_back(rightRail_v7);  rightRail_N.push_back(-1.0f*binormals[j]);
    // right face
    // triangle 1
    rightRail.push_back(rightRail_v0);  rightRail_N.push_back(binormals[i]);
    rightRail.push_back(rightRail_v4);  rightRail_N.push_back(binormals[j]);
    rightRail.push_back(rightRail_v5);  rightRail_N.push_back(binormals[j]);
    // triangle 2
    rightRail.push_back(rightRail_v0);  rightRail_N.push_back(binormals[i]);
    rightRail.push_back(rightRail_v5);  rightRail_N.push_back(binormals[j]);
    rightRail.push_back(rightRail_v1);  rightRail_N.push_back(binormals[i]);
    // top face
    // triangle 1
    rightRail.push_back(rightRail_v1);  rightRail_N.push_back(normals[i]);
    rightRail.push_back(rightRail_v5);  rightRail_N.push_back(normals[j]);
    rightRail.push_back(rightRail_v2);  rightRail_N.push_back(normals[i]);
    // triangle 2
    rightRail.push_back(rightRail_v2);  rightRail_N.push_back(normals[i]);
    rightRail.push_back(rightRail_v5);  rightRail_N.push_back(normals[j]);
    rightRail.push_back(rightRail_v6);  rightRail_N.push_back(normals[j]);
    // bottom face
    // triangle 1
    rightRail.push_back(leftRail_v3);  rightRail_N.push_back(-1.0f*normals[i]);
    rightRail.push_back(leftRail_v7);  rightRail_N.push_back(-1.0f*normals[j]);
    rightRail.push_back(leftRail_v4);  rightRail_N.push_back(-1.0f*normals[j]);
    // triangle 2
    rightRail.push_back(leftRail_v3);  rightRail_N.push_back(-1.0f*normals[i]);
    rightRail.push_back(leftRail_v4);  rightRail_N.push_back(-1.0f*normals[j]);
    rightRail.push_back(leftRail_v0);  rightRail_N.push_back(-1.0f*normals[i]);
    // For right lowerT
    // left face
    // triangle 1
    rightRail.push_back(rightLowerT_v2);  rightRail_N.push_back(-1.0f*binormals[i]);
    rightRail.push_back(rightLowerT_v6);  rightRail_N.push_back(-1.0f*binormals[j]);
    rightRail.push_back(rightLowerT_v3);  rightRail_N.push_back(-1.0f*binormals[i]);
    // triangle 2
    rightRail.push_back(rightLowerT_v3);  rightRail_N.push_back(-1.0f*binormals[i]);
    rightRail.push_back(rightLowerT_v6);  rightRail_N.push_back(-1.0f*binormals[j]);
    rightRail.push_back(rightLowerT_v7);  rightRail_N.push_back(-1.0f*binormals[j]);
    // right face
    // triangle 1
    rightRail.push_back(rightLowerT_v0);  rightRail_N.push_back(binormals[i]);
    rightRail.push_back(rightLowerT_v4);  rightRail_N.push_back(binormals[j]);
    rightRail.push_back(rightLowerT_v5);  rightRail_N.push_back(binormals[j]);
    // triangle 2
    rightRail.push_back(rightLowerT_v0);  rightRail_N.push_back(binormals[i]);
    rightRail.push_back(rightLowerT_v5);  rightRail_N.push_back(binormals[j]);
    rightRail.push_back(rightLowerT_v1);  rightRail_N.push_back(binormals[i]);
    // top face
    // triangle 1
    rightRail.push_back(rightLowerT_v1);  rightRail_N.push_back(normals[i]);
    rightRail.push_back(rightLowerT_v5);  rightRail_N.push_back(normals[j]);
    rightRail.push_back(rightLowerT_v2);  rightRail_N.push_back(normals[i]);
    // triangle 2
    rightRail.push_back(rightLowerT_v2);  rightRail_N.push_back(normals[i]);
    rightRail.push_back(rightLowerT_v5);  rightRail_N.push_back(normals[j]);
    rightRail.push_back(rightLowerT_v6);  rightRail_N.push_back(normals[j]);
    // bottom face
    // triangle 1
    rightRail.push_back(leftLowerT_v3);  rightRail_N.push_back(-1.0f*normals[i]);
    rightRail.push_back(leftLowerT_v7);  rightRail_N.push_back(-1.0f*normals[j]);
    rightRail.push_back(leftLowerT_v4);  rightRail_N.push_back(-1.0f*normals[j]);
    // triangle 2
    rightRail.push_back(leftLowerT_v3);  rightRail_N.push_back(-1.0f*normals[i]);
    rightRail.push_back(leftLowerT_v4);  rightRail_N.push_back(-1.0f*normals[j]);
    rightRail.push_back(leftLowerT_v0);  rightRail_N.push_back(-1.0f*normals[i]);


    // For Support
    if (i % 60 == 0)
    {
      // Left rail support vertices                                                         // Right rail support vertices
      leftSupport_v0 = leftSupport_v3 = leftRail_v3;                                        rightSupport_v0 = rightSupport_v3 = rightRail_v3;
      leftSupport_v1 = leftSupport_v2 = leftRail_v2;                                        rightSupport_v1 = rightSupport_v2 = rightRail_v2;
      leftSupport_v4 = leftSupport_v7 = leftRail_v7;                                        rightSupport_v4 = rightSupport_v7 = rightRail_v7;
      leftSupport_v5 = leftSupport_v6 = leftRail_v6;                                        rightSupport_v5 = rightSupport_v6 = rightRail_v6;
      leftSupport_v2.y = leftSupport_v3.y = leftSupport_v6.y = leftSupport_v7.y = d;        rightSupport_v2.y = rightSupport_v3.y = rightSupport_v6.y = rightSupport_v7.y = d;
      
      // For the left support
      // left face
      // triangle 1
      leftSupport.push_back(leftSupport_v2);  leftSupport_N.push_back(-1.0f*binormals[i]);
      leftSupport.push_back(leftSupport_v6);  leftSupport_N.push_back(-1.0f*binormals[j]);
      leftSupport.push_back(leftSupport_v3);  leftSupport_N.push_back(-1.0f*binormals[i]);
      // triangle 2
      leftSupport.push_back(leftSupport_v3);  leftSupport_N.push_back(-1.0f*binormals[i]);
      leftSupport.push_back(leftSupport_v6);  leftSupport_N.push_back(-1.0f*binormals[j]);
      leftSupport.push_back(leftSupport_v7);  leftSupport_N.push_back(-1.0f*binormals[j]);
      // right face
      // triangle 1
      leftSupport.push_back(leftSupport_v0);  leftSupport_N.push_back(binormals[i]);
      leftSupport.push_back(leftSupport_v4);  leftSupport_N.push_back(binormals[j]);
      leftSupport.push_back(leftSupport_v5);  leftSupport_N.push_back(binormals[j]);
      // triangle 2
      leftSupport.push_back(leftSupport_v0);  leftSupport_N.push_back(binormals[i]);
      leftSupport.push_back(leftSupport_v5);  leftSupport_N.push_back(binormals[j]);
      leftSupport.push_back(leftSupport_v1);  leftSupport_N.push_back(binormals[i]);
      // front face
      // triangle 1
      leftSupport.push_back(leftSupport_v0);
      leftSupport.push_back(leftSupport_v2);
      leftSupport.push_back(leftSupport_v3);
      // triangle 2
      leftSupport.push_back(leftSupport_v0);
      leftSupport.push_back(leftSupport_v1);
      leftSupport.push_back(leftSupport_v2);
      for (int k=0; k<6; k++) leftSupport_N.push_back(-1.0f*tangents[i]);
      // back face
      // triangle 1
      leftSupport.push_back(leftSupport_v4);
      leftSupport.push_back(leftSupport_v6);
      leftSupport.push_back(leftSupport_v5);
      // triangle 2
      leftSupport.push_back(leftSupport_v7);
      leftSupport.push_back(leftSupport_v6);
      leftSupport.push_back(leftSupport_v4);
      for (int k=0; k<6; k++) leftSupport_N.push_back(tangents[j]);
      // top face
      // triangle 1
      leftSupport.push_back(leftSupport_v1);  leftSupport_N.push_back(normals[i]);
      leftSupport.push_back(leftSupport_v5);  leftSupport_N.push_back(normals[j]);
      leftSupport.push_back(leftSupport_v2);  leftSupport_N.push_back(normals[i]);
      // triangle 2
      leftSupport.push_back(leftSupport_v2);  leftSupport_N.push_back(normals[i]);
      leftSupport.push_back(leftSupport_v5);  leftSupport_N.push_back(normals[j]);
      leftSupport.push_back(leftSupport_v6);  leftSupport_N.push_back(normals[j]);
      // bottom face
      // triangle 1
      leftSupport.push_back(leftSupport_v3);  leftSupport_N.push_back(-1.0f*normals[i]);
      leftSupport.push_back(leftSupport_v7);  leftSupport_N.push_back(-1.0f*normals[j]);
      leftSupport.push_back(leftSupport_v4);  leftSupport_N.push_back(-1.0f*normals[j]);
      // triangle 2
      leftSupport.push_back(leftSupport_v3);  leftSupport_N.push_back(-1.0f*normals[i]);
      leftSupport.push_back(leftSupport_v4);  leftSupport_N.push_back(-1.0f*normals[j]);
      leftSupport.push_back(leftSupport_v0);  leftSupport_N.push_back(-1.0f*normals[i]);

      // For the right support
      // left face
      // triangle 1
      rightSupport.push_back(rightSupport_v2);  rightSupport_N.push_back(-1.0f*binormals[i]);
      rightSupport.push_back(rightSupport_v6);  rightSupport_N.push_back(-1.0f*binormals[j]);
      rightSupport.push_back(rightSupport_v3);  rightSupport_N.push_back(-1.0f*binormals[i]);
      // triangle 2
      rightSupport.push_back(rightSupport_v3);  rightSupport_N.push_back(-1.0f*binormals[i]);
      rightSupport.push_back(rightSupport_v6);  rightSupport_N.push_back(-1.0f*binormals[j]);
      rightSupport.push_back(rightSupport_v7);  rightSupport_N.push_back(-1.0f*binormals[j]);
      // right face
      // triangle 1
      rightSupport.push_back(rightSupport_v0);  rightSupport_N.push_back(binormals[i]);
      rightSupport.push_back(rightSupport_v4);  rightSupport_N.push_back(binormals[j]);
      rightSupport.push_back(rightSupport_v5);  rightSupport_N.push_back(binormals[j]);
      // triangle 2
      rightSupport.push_back(rightSupport_v0);  rightSupport_N.push_back(binormals[i]);
      rightSupport.push_back(rightSupport_v5);  rightSupport_N.push_back(binormals[j]);
      rightSupport.push_back(rightSupport_v1);  rightSupport_N.push_back(binormals[i]);
      // front face
      // triangle 1
      rightSupport.push_back(rightSupport_v0);
      rightSupport.push_back(rightSupport_v2);
      rightSupport.push_back(rightSupport_v3);
      // triangle 2
      rightSupport.push_back(rightSupport_v0);
      rightSupport.push_back(rightSupport_v1);
      rightSupport.push_back(rightSupport_v2);
      for (int k=0; k<6; k++) rightSupport_N.push_back(-1.0f*tangents[i]);
      // back face
      // triangle 1
      rightSupport.push_back(rightSupport_v4);
      rightSupport.push_back(rightSupport_v6);
      rightSupport.push_back(rightSupport_v5);
      // triangle 2
      rightSupport.push_back(rightSupport_v7);
      rightSupport.push_back(rightSupport_v6);
      rightSupport.push_back(rightSupport_v4);
      for (int k=0; k<6; k++) rightSupport_N.push_back(tangents[j]);
      // top face
      // triangle 1
      rightSupport.push_back(rightSupport_v1);  rightSupport_N.push_back(normals[i]);
      rightSupport.push_back(rightSupport_v5);  rightSupport_N.push_back(normals[j]);
      rightSupport.push_back(rightSupport_v2);  rightSupport_N.push_back(normals[i]);
      // triangle 2
      rightSupport.push_back(rightSupport_v2);  rightSupport_N.push_back(normals[i]);
      rightSupport.push_back(rightSupport_v5);  rightSupport_N.push_back(normals[j]);
      rightSupport.push_back(rightSupport_v6);  rightSupport_N.push_back(normals[j]);
      // bottom face
      // triangle 1
      rightSupport.push_back(rightSupport_v3);  rightSupport_N.push_back(-1.0f*normals[i]);
      rightSupport.push_back(rightSupport_v7);  rightSupport_N.push_back(-1.0f*normals[j]);
      rightSupport.push_back(rightSupport_v4);  rightSupport_N.push_back(-1.0f*normals[j]);
      // triangle 2
      rightSupport.push_back(rightSupport_v3);  rightSupport_N.push_back(-1.0f*normals[i]);
      rightSupport.push_back(rightSupport_v4);  rightSupport_N.push_back(-1.0f*normals[j]);
      rightSupport.push_back(rightSupport_v0);  rightSupport_N.push_back(-1.0f*normals[i]);
    }
  }
}

void initVBO_VAO_RailsAndSupports()
{
  // FOR MILESTONE
  // // For lines
  // glGenBuffers(1, &vboSpline); // get handle on VBO buffer
  // glBindBuffer(GL_ARRAY_BUFFER, vboSpline);  // bind the VBO buffer
  // // glBufferData() can allocate memory but glBufferSubData() cannot
  // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * splinePoints.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * splinePoints.size(), (glm::vec3*)(splinePoints.data()));

  // glGenVertexArrays(1, &vaoSpline);
  // glBindVertexArray(vaoSpline); // bind the VAO
  // glBindBuffer(GL_ARRAY_BUFFER, vboSpline);
  // // get location index of the "position" shader variable
  // GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  // glEnableVertexAttribArray(loc); // enable the "position" attribute
  // const void* offset = (const void*) 0;
  // glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data
  // glBindVertexArray(0); // unbind the VAO

  // VBOs
  glGenBuffers(1, &vboLeftRail); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboLeftRail);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * leftRail.size(), leftRail.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboLeftRail_N); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboLeftRail_N);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * leftRail_N.size(), leftRail_N.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboSleeper); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSleeper);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * sleeper.size(), sleeper.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboSleeper_N); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSleeper_N);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * sleeper_N.size(), sleeper_N.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboRightRail); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboRightRail);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * rightRail.size(), rightRail.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboRightRail_N); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboRightRail_N);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * rightRail_N.size(), rightRail_N.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboLeftSupport); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboLeftSupport);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * leftSupport.size(), leftSupport.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboLeftSupport_N); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboLeftSupport_N);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * leftSupport_N.size(), leftSupport_N.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboRightSupport); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboRightSupport);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * rightSupport.size(), rightSupport.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboRightSupport_N); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboRightSupport_N);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * rightSupport_N.size(), rightSupport_N.data(), GL_STATIC_DRAW); // to allocate memory
  

  // VAOs

  // left rail
  glGenVertexArrays(1, &vaoLeftRail);
  glBindVertexArray(vaoLeftRail); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboLeftRail);
  // get location index of the "position" shader variable
  GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  const void* offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  glBindBuffer(GL_ARRAY_BUFFER, vboLeftRail_N);
  // get location index of the "normal" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "normal" attribute data

  // sleeper
  glGenVertexArrays(1, &vaoSleeper);
  glBindVertexArray(vaoSleeper); // bind the VAO

  glBindBuffer(GL_ARRAY_BUFFER, vboSleeper);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  glBindBuffer(GL_ARRAY_BUFFER, vboSleeper_N);
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc); // enable the "normal" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "normal" attribute data

  // right rail
  glGenVertexArrays(1, &vaoRightRail);
  glBindVertexArray(vaoRightRail); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboRightRail);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  glBindBuffer(GL_ARRAY_BUFFER, vboRightRail_N);
  // get location index of the "normal" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "normal" attribute data

  // left rail support
  glGenVertexArrays(1, &vaoLeftSupport);
  glBindVertexArray(vaoLeftSupport); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboLeftSupport);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  glBindBuffer(GL_ARRAY_BUFFER, vboLeftSupport_N);
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc); // enable the "normal" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "normal" attribute data

  // right rail support
  glGenVertexArrays(1, &vaoRightSupport);
  glBindVertexArray(vaoRightSupport); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboRightSupport);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  glBindBuffer(GL_ARRAY_BUFFER, vboRightSupport_N);
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc); // enable the "normal" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "normal" attribute data

  glBindVertexArray(0); // unbind the VAO
}

void skyAndGround()
{
  // float d = 150.0f; - declared globally

  // left face
  skyboxLPos.push_back(glm::vec3(-d, d, d));
  skyboxLUV.push_back(glm::vec2(0,0));
  skyboxLPos.push_back(glm::vec3(-d, -d, d));
  skyboxLUV.push_back(glm::vec2(0,1));
  skyboxLPos.push_back(glm::vec3(-d, -d, -d));
  skyboxLUV.push_back(glm::vec2(1,1));

  skyboxLPos.push_back(glm::vec3(-d, d, d));
  skyboxLUV.push_back(glm::vec2(0,0));
  skyboxLPos.push_back(glm::vec3(-d, -d, -d));
  skyboxLUV.push_back(glm::vec2(1,1));
  skyboxLPos.push_back(glm::vec3(-d, d, -d));
  skyboxLUV.push_back(glm::vec2(1,0));

  // right face
  skyboxRPos.push_back(glm::vec3(d, d, -d));
  skyboxRUV.push_back(glm::vec2(0,0));
  skyboxRPos.push_back(glm::vec3(d, -d, -d));
  skyboxRUV.push_back(glm::vec2(0,1));
  skyboxRPos.push_back(glm::vec3(d, -d, d));
  skyboxRUV.push_back(glm::vec2(1,1));

  skyboxRPos.push_back(glm::vec3(d, d, -d));
  skyboxRUV.push_back(glm::vec2(0,0));
  skyboxRPos.push_back(glm::vec3(d, -d, d));
  skyboxRUV.push_back(glm::vec2(1,1));
  skyboxRPos.push_back(glm::vec3(d, d, d));
  skyboxRUV.push_back(glm::vec2(1,0));

  // front face
  skyboxFPos.push_back(glm::vec3(d, d, d));
  skyboxFUV.push_back(glm::vec2(0,0));
  skyboxFPos.push_back(glm::vec3(d, -d, d));
  skyboxFUV.push_back(glm::vec2(0,1));
  skyboxFPos.push_back(glm::vec3(-d, -d, d));
  skyboxFUV.push_back(glm::vec2(1,1));

  skyboxFPos.push_back(glm::vec3(d, d, d));
  skyboxFUV.push_back(glm::vec2(0,0));
  skyboxFPos.push_back(glm::vec3(-d, -d, d));
  skyboxFUV.push_back(glm::vec2(1,1));
  skyboxFPos.push_back(glm::vec3(-d, d, d));
  skyboxFUV.push_back(glm::vec2(1,0));

  // back face
  skyboxBPos.push_back(glm::vec3(-d, d, -d));
  skyboxBUV.push_back(glm::vec2(0,0));
  skyboxBPos.push_back(glm::vec3(-d, -d, -d));
  skyboxBUV.push_back(glm::vec2(0,1));
  skyboxBPos.push_back(glm::vec3(d, -d, -d));
  skyboxBUV.push_back(glm::vec2(1,1));

  skyboxBPos.push_back(glm::vec3(-d, d, -d));
  skyboxBUV.push_back(glm::vec2(0,0));
  skyboxBPos.push_back(glm::vec3(d, -d, -d));
  skyboxBUV.push_back(glm::vec2(1,1));
  skyboxBPos.push_back(glm::vec3(d, d, -d));
  skyboxBUV.push_back(glm::vec2(1,0));

  // bottom(down) face - ground
  skyboxDPos.push_back(glm::vec3(-d, d, d));  // ground
  skyboxDUV.push_back(glm::vec2(1,1));
  skyboxDPos.push_back(glm::vec3(-d, d, -d)); // ground
  skyboxDUV.push_back(glm::vec2(1,0));
  skyboxDPos.push_back(glm::vec3(d, d, -d));  // ground
  skyboxDUV.push_back(glm::vec2(0,0));

  skyboxDPos.push_back(glm::vec3(-d, d, d));  // ground
  skyboxDUV.push_back(glm::vec2(1,1));
  skyboxDPos.push_back(glm::vec3(d, d, -d));  // ground
  skyboxDUV.push_back(glm::vec2(0,0));
  skyboxDPos.push_back(glm::vec3(d, d, d));   // ground
  skyboxDUV.push_back(glm::vec2(0,1));

  // top face
  skyboxTPos.push_back(glm::vec3(-d, -d, -d));
  skyboxTUV.push_back(glm::vec2(1,1));
  skyboxTPos.push_back(glm::vec3(-d, -d, d));
  skyboxTUV.push_back(glm::vec2(1,0));
  skyboxTPos.push_back(glm::vec3(d, -d, d));
  skyboxTUV.push_back(glm::vec2(0,0));

  skyboxTPos.push_back(glm::vec3(-d, -d, -d));
  skyboxTUV.push_back(glm::vec2(1,1));
  skyboxTPos.push_back(glm::vec3(d, -d, d));
  skyboxTUV.push_back(glm::vec2(0,0));
  skyboxTPos.push_back(glm::vec3(d, -d, -d));
  skyboxTUV.push_back(glm::vec2(0,1));
}

void initVBO_VAO_skyAndGround()
{
  // VBOs
  glGenBuffers(1, &vboSkyboxL); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxL);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxLPos.size() + sizeof(glm::vec2) * skyboxLUV.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * skyboxLPos.size(), (glm::vec3*)(skyboxLPos.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxLPos.size(), sizeof(glm::vec2) * skyboxLUV.size(), (glm::vec2*)(skyboxLUV.data()));

  glGenBuffers(1, &vboSkyboxR); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxR);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxRPos.size() + sizeof(glm::vec2) * skyboxRUV.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * skyboxRPos.size(), (glm::vec3*)(skyboxRPos.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxRPos.size(), sizeof(glm::vec2) * skyboxRUV.size(), (glm::vec2*)(skyboxRUV.data()));

  glGenBuffers(1, &vboSkyboxF); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxF);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxFPos.size() + sizeof(glm::vec2) * skyboxFUV.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * skyboxFPos.size(), (glm::vec3*)(skyboxFPos.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxFPos.size(), sizeof(glm::vec2) * skyboxFUV.size(), (glm::vec2*)(skyboxFUV.data()));

  glGenBuffers(1, &vboSkyboxB); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxB);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxBPos.size() + sizeof(glm::vec2) * skyboxBUV.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * skyboxBPos.size(), (glm::vec3*)(skyboxBPos.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxBPos.size(), sizeof(glm::vec2) * skyboxBUV.size(), (glm::vec2*)(skyboxBUV.data()));

  glGenBuffers(1, &vboSkyboxD); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxD);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxDPos.size() + sizeof(glm::vec2) * skyboxDUV.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * skyboxDPos.size(), (glm::vec3*)(skyboxDPos.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxDPos.size(), sizeof(glm::vec2) * skyboxDUV.size(), (glm::vec2*)(skyboxDUV.data()));

  glGenBuffers(1, &vboSkyboxT); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxT);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxTPos.size() + sizeof(glm::vec2) * skyboxTUV.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * skyboxTPos.size(), (glm::vec3*)(skyboxTPos.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * skyboxTPos.size(), sizeof(glm::vec2) * skyboxTUV.size(), (glm::vec2*)(skyboxTUV.data()));

  // VAOs
  glGenVertexArrays(1, &vaoSkyboxL);
  glBindVertexArray(vaoSkyboxL); // bind the VAO

  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxL);
  // get location index of the "position" shader variable
  GLuint loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  const void* offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  // get location index of the "texCoord" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc); // enable the "texCoord" attribute
  offset = (const void*) (sizeof(glm::vec3) * skyboxLPos.size());
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "texCoord" attribute data

  glGenVertexArrays(1, &vaoSkyboxR);
  glBindVertexArray(vaoSkyboxR); // bind the VAO

  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxR);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  // get location index of the "texCoord" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc); // enable the "texCoord" attribute
  offset = (const void*) (sizeof(glm::vec3) * skyboxRPos.size());
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "texCoord" attribute data

  glGenVertexArrays(1, &vaoSkyboxF);
  glBindVertexArray(vaoSkyboxF); // bind the VAO

  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxF);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  // get location index of the "texCoord" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc); // enable the "texCoord" attribute
  offset = (const void*) (sizeof(glm::vec3) * skyboxFPos.size());
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "texCoord" attribute data

  glGenVertexArrays(1, &vaoSkyboxB);
  glBindVertexArray(vaoSkyboxB); // bind the VAO

  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxB);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  // get location index of the "texCoord" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc); // enable the "texCoord" attribute
  offset = (const void*) (sizeof(glm::vec3) * skyboxBPos.size());
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "texCoord" attribute data

  glGenVertexArrays(1, &vaoSkyboxD);
  glBindVertexArray(vaoSkyboxD); // bind the VAO

  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxD);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  // get location index of the "texCoord" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc); // enable the "texCoord" attribute
  offset = (const void*) (sizeof(glm::vec3) * skyboxDPos.size());
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "texCoord" attribute data

  glGenVertexArrays(1, &vaoSkyboxT);
  glBindVertexArray(vaoSkyboxT); // bind the VAO

  glBindBuffer(GL_ARRAY_BUFFER, vboSkyboxT);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  // get location index of the "texCoord" shader variable
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc); // enable the "texCoord" attribute
  offset = (const void*) (sizeof(glm::vec3) * skyboxTPos.size());
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "texCoord" attribute data
}

void initVBO_VAO_OBJ()  // For OBJ rendering
{
  // VBOs
  glGenBuffers(1, &vboOBJvertices); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboOBJvertices);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * OBJvertices.size(), OBJvertices.data(), GL_STATIC_DRAW); // to allocate memory

  glGenBuffers(1, &vboOBJnormals); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboOBJnormals);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * OBJnormals.size(), OBJnormals.data(), GL_STATIC_DRAW); // to allocate memory

  // left rail
  glGenVertexArrays(1, &vaoOBJ);
  glBindVertexArray(vaoOBJ); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboOBJvertices);
  // get location index of the "position" shader variable
  GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  const void* offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data

  glBindBuffer(GL_ARRAY_BUFFER, vboOBJnormals);
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc); // enable the "normal" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "normal" attribute data
}

void createTexHandles()
{
  // create integer handles for the textures
  glGenTextures(1, &texHandleL);

  int code = initTexture("skybox/posx.jpg", texHandleL);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleR);

  code = initTexture("skybox/negx.jpg", texHandleR);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleF);

  code = initTexture("skybox/posz.jpg", texHandleF);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleB);

  code = initTexture("skybox/negz.jpg", texHandleB);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleD);

  code = initTexture("skybox/negy.jpg", texHandleD);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleT);

  code = initTexture("skybox/posy.jpg", texHandleT);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }
}

void initScene(int argc, char *argv[])
{
  // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearColor(0.08f, 0.0f, 0.08f, 0.0f);

  getPointsAndTNB();
  buildRailsAndSupports();

  // getPoints(); // FOR MILESTONE

  // glGenBuffers(1, &triVertexBuffer);
  // glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 3, triangle,
  //              GL_STATIC_DRAW);

  // glGenBuffers(1, &triColorVertexBuffer);
  // glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * 3, color, GL_STATIC_DRAW);

  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  pipelineProgram->Bind();

  initVBO_VAO_RailsAndSupports();



  bool res = myOBJ.readOBJfile("objects/dragon-77k.obj", OBJvertices, OBJnormals);

  OBJPipelineProgram = new BasicPipelineProgram;
  ret = OBJPipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  OBJPipelineProgram->Bind();

  initVBO_VAO_OBJ();


  skyAndGround();

  texturePipelineProgram = new TexPipelineProgram;
  ret = texturePipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  texturePipelineProgram->Bind();

  // create integer handles for the textures
  createTexHandles();

  initVBO_VAO_skyAndGround();
  
  // initVBO_VAO(); // FOR MILESTONE

  // initVBO_VAO_mode0();

  // initVBO_VAO_mode1();

  // glGenVertexArrays(1, &triVertexArray);
  // glBindVertexArray(triVertexArray);
  // glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer);

  // GLuint loc =
  //     glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  // glEnableVertexAttribArray(loc);
  // glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  // glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer);
  // loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  // glEnableVertexAttribArray(loc);
  // glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glEnable(GL_DEPTH_TEST);

  // sizeTri = 3;

  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc<2)
  {  
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }
  // ARGC = argc;

  // load the splines from the provided filename
  loadSplines(argv[1]);

  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}