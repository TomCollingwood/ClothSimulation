#include "ClothScene.h"

#include <glm/gtc/type_ptr.hpp>
#include <ngl/Obj.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>

ClothScene::ClothScene() : Scene() {}

/**
 * @brief ObjLoaderScene::initGL
 */
void ClothScene::initGL() noexcept {
    // Fire up the NGL machinary (not doing this will make it crash)
    ngl::NGLInit::instance();

    // Set background colour
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);

    // enable depth testing for drawing
    glEnable(GL_DEPTH_TEST);

    // enable multisampling for smoother drawing
    glEnable(GL_MULTISAMPLE);

    // Create and compile all of our shaders
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    shader->loadShader("GouraudProgram","../common/shaders/gouraud_vert.glsl","../common/shaders/gouraud_frag.glsl");
    shader->loadShader("PhongProgram","shaders/phong_vert.glsl","shaders/phong_frag.glsl");
    shader->loadShader("CookTorranceProgram","shaders/phong_vert.glsl","shaders/cooktorrance_frag.glsl");
    shader->loadShader("ToonProgram","shaders/phong_vert.glsl","shaders/toon_frag.glsl");

    glGenVertexArrays(1, &vertexArrayIdx);
    glBindVertexArray(vertexArrayIdx);

    initSpringsAndVerts();
    initTriangles();
    initVertexBuffers();

    glBindVertexArray(0);
}

void ClothScene::paintGL() noexcept {
    // Clear the screen (fill with our glClearColor)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the viewport
    glViewport(0,0,m_width,m_height);

    // Use our shader for this draw
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    GLint pid;

    // Allow the user to select the current shader method
    switch(m_shaderMethod) {
    case SHADER_PHONG:
        (*shader)["PhongProgram"]->use();
        pid = shader->getProgramID("PhongProgram");
        break;
    case SHADER_TOON:
        (*shader)["ToonProgram"]->use();
        pid = shader->getProgramID("ToonProgram");
        break;
    case SHADER_COOKTORRANCE:
        (*shader)["CookTorranceProgram"]->use();
        pid = shader->getProgramID("CookTorranceProgram");
        break;
    default:
        (*shader)["GouraudProgram"]->use();
        pid = shader->getProgramID("GouraudProgram");
        break;
    }

    // Our MVP matrices
    glm::mat4 M = glm::mat4(1.0f);
    glm::mat4 MVP, MV;
    glm::mat3 N;

    // Note the matrix multiplication order as we are in COLUMN MAJOR storage
    MV = m_V * M;
    N = glm::inverse(glm::mat3(MV));
    MVP = m_P * MV;

    // Set this MVP on the GPU
    glUniformMatrix4fv(glGetUniformLocation(pid, "MVP"), //location of uniform
                       1, // how many matrices to transfer
                       false, // whether to transpose matrix
                       glm::value_ptr(MVP)); // a raw pointer to the data
    glUniformMatrix4fv(glGetUniformLocation(pid, "MV"), //location of uniform
                       1, // how many matrices to transfer
                       false, // whether to transpose matrix
                       glm::value_ptr(MV)); // a raw pointer to the data
    glUniformMatrix3fv(glGetUniformLocation(pid, "N"), //location of uniform
                       1, // how many matrices to transfer
                       true, // whether to transpose matrix
                       glm::value_ptr(N)); // a raw pointer to the data


    glBindVertexArray(vertexArrayIdx);

    glBindBuffer(GL_ARRAY_BUFFER, posIdx); // Bind it (all following operations apply)
    glBufferSubData(GL_ARRAY_BUFFER,0,res*res*sizeof(glm::vec3),&vertexPositions[0]);
    glBindBuffer(GL_ARRAY_BUFFER, normalsIdx); // Bind it (all following operations apply)
    glBufferSubData(GL_ARRAY_BUFFER,0,res*res*sizeof(glm::vec3),&vertexNormals[0]);

    // Retrieve the attribute location from our currently bound shader, enable and
    // bind the vertex attrib pointer to our currently bound buffer
    glBindBuffer(GL_ARRAY_BUFFER, posIdx); // Bind it (all following operations apply)
    GLint vertAttribLoc = glGetAttribLocation(pid, "VertexPosition");
    glEnableVertexAttribArray(vertAttribLoc);
    glVertexAttribPointer(vertAttribLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Do the same for the normals
    glBindBuffer(GL_ARRAY_BUFFER, normalsIdx);
    GLint normAttribLoc = glGetAttribLocation(pid, "VertexNormal");
    glEnableVertexAttribArray(normAttribLoc);
    glVertexAttribPointer(normAttribLoc, 3, GL_FLOAT, GL_TRUE, 0, 0);

    // Draw our elements (the element buffer should still be enabled from the previous call to initScene())
    unsigned int num_tris = (res-1)*(res-1)*2;
    unsigned int num_points = res * res;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsIdx);
    glDrawElements(GL_TRIANGLES, num_tris*3, GL_UNSIGNED_INT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, posIdx);
    glDrawArrays(GL_POINTS, 0, num_points);

    glBindVertexArray(0);

    //ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
    //prim->draw("teapot");
}

void ClothScene::initVertexBuffers()
{
  // Create our GL buffers and bind them to CUDA
  glGenBuffers(1, &posIdx); // Generate the point buffer index
  glBindBuffer(GL_ARRAY_BUFFER, posIdx); // Bind it (all following operations apply)
  glBufferData(GL_ARRAY_BUFFER, res*res*sizeof(glm::vec3), &vertexPositions[0], GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind our buffers
  // Now do the same for the normals
  glGenBuffers(1, &normalsIdx);
  glBindBuffer(GL_ARRAY_BUFFER, normalsIdx);
  glBufferData(GL_ARRAY_BUFFER, res*res*sizeof(glm::vec3), &vertexNormals[0], GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ClothScene::initTriangles()
{
  // Define some connectivity information for our sheet.
  unsigned int num_tris = (res-1)*(res-1)*2;
  GLuint *tris = new GLuint[num_tris*3];
  int i, j, fidx = 0;
  for (i=0; i < res - 1; ++i) {
      for (j=0; j < res - 1; ++j) {
          tris[fidx*3+0] = i*res+j; tris[fidx*3+1] = i*res+j+1; tris[fidx*3+2] = (i+1)*res+j;
          fidx++;
          tris[fidx*3+0] = i*res+j+1; tris[fidx*3+1] = (i+1)*res+j+1; tris[fidx*3+2] = (i+1)*res+j;
          fidx++;
      }
  }

  // Create our buffer to contain element data (we will use indexed arrays to draw this shape)
  glGenBuffers(1, &elementsIdx);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsIdx);

  // Note that in this call we copy the data to the GPU from the tris array
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*num_tris*sizeof(GLuint), tris, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  delete [] tris; // We can delete this array - it has been copied to the GPU
}

void ClothScene::initSpringsAndVerts()
{
  vertexPositions.resize(res*res);
  vertexNormals.resize(res*res);
  for(int i=0; i<res; ++i)
  {
    for(int j=0; j<res; ++j)
    {
      vertexPositions[i*res + j]=glm::vec3(float(i)/float(res),float(j)/float(res),0.0f);
      vertexNormals[i*res + j] = glm::vec3(0.0f,0.0f,1.0f);
    }
  }
}
