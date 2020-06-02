#include "texPipelineProgram.h"
#include "openGLHeader.h"
#include <iostream>
#include <cstring>

using namespace std;

int TexPipelineProgram::Init(const char * shaderBasePath) 
{
  if (BuildShadersFromFiles(shaderBasePath, "tex.vertexShader.glsl", "tex.fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the texture pipeline program." << endl;
    return 1;
  }

  cout << "Successfully built the texture pipeline program." << endl;
  return 0;
}

void TexPipelineProgram::SetModelViewMatrix(const float * m) 
{
  // Pass "m" to the pipeline program, as the modelview matrix.
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
}

void TexPipelineProgram::SetProjectionMatrix(const float * m) 
{
  // Pass "m" to the pipeline program, as the projection matrix.
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, m);
}

int TexPipelineProgram::SetShaderVariableHandles() 
{
  // Set h_modelViewMatrix and h_projectionMatrix.
  SET_SHADER_VARIABLE_HANDLE(modelViewMatrix);
  SET_SHADER_VARIABLE_HANDLE(projectionMatrix);
  return 0;
}

