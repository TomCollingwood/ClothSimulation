#include "ClothScene.h"

#include <glm/gtc/type_ptr.hpp>
#include <ngl/Obj.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <math.h>
#include <time.h>

//#define _FORCES_

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

    m_sphere.reset(new ngl::Obj("models/sphere.obj"));
    m_sphere->createVAO();

    last_time = clock();
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

    loadMatricesToShader(pid,glm::vec3(0.0f,0.0f,0.0f));

    updateSimulation(EULER);

    updateNormals();

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

    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsIdx);
    glDrawElements(GL_TRIANGLES, num_tris*3, GL_UNSIGNED_INT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, posIdx);
    glDrawArrays(GL_POINTS, 0, num_points);

    glBindVertexArray(0);


    loadMatricesToShader(pid,sphereTranslation);
    m_sphere->draw();

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
  pointMasses.resize(res*res);
  m_forces.resize(res*res);

  //--------------------------STRUCTURAL SPRINGS---------------

  for(int i=0; i<res; ++i)
  {
    for(int j=0; j<res; ++j)
    {
      m_forces[i*res + j] = glm::vec3(0.0f);
      // j is x
      // i is y
      vertexPositions[i*res + j]=glm::vec3(float(j)/float(res),float(i)/float(res),0.0f);

      //if(i==0 && j==0) std::cout<<"0,0: "<<vertexPositions[i*res + j].x<<","<<vertexPositions[i*res + j].y<<","<<vertexPositions[i*res + j].z<<"\n";
      //if(i==0 && j==res-1) std::cout<<"0,0: "<<vertexPositions[i*res + j].x<<","<<vertexPositions[i*res + j].y<<","<<vertexPositions[i*res + j].z<<"\n";

      vertexNormals[i*res + j] = glm::vec3(0.0f,0.0f,1.0f);
      PointMass newPoint;
      newPoint.velocity = glm::vec3(0.0f);
      newPoint.prevPos = glm::vec3(float(j)/float(res),float(i)/float(res),0.0f);
      newPoint.index = i*res + j;
      newPoint.mass = 1.0f;
      pointMasses[i*res + j]=newPoint;
      //-----------SETUP SPRINGS---------------
      if(j!=(res-1) && i!=(res-1))
      {
        Spring newLink1;
        newLink1.PointMassA = i*res+j;
        newLink1.PointMassB = i*res+j+1;
        newLink1.restingDistance=1.0f/float(res);
        newLink1.stiffness=0.5f;
        newLink1.damping=0.5f;
        m_springs.push_back(newLink1);
        Spring newLink2;
        newLink2.PointMassA = i*res+j;
        newLink2.PointMassB = (i+1)*res+j;
        newLink2.restingDistance=1.0f/float(res);
        newLink2.stiffness=0.5f;
        newLink2.damping=0.5f;
        m_springs.push_back(newLink2);
      }
      //-------------FAR LEFT-------------------
      else if(j==(res-1) && i!=(res-1))
      {
        Spring newLink2;
        newLink2.PointMassA = i*res+j;
        newLink2.PointMassB = (i+1)*res+j;
        newLink2.restingDistance=1.0f/float(res);
        newLink2.stiffness=0.5f;
        newLink2.damping=0.5f;
        m_springs.push_back(newLink2);
      }
      //-------------TOP ROW------------------
      else if(i==(res-1)&&j!=(res-1))
      {
        Spring newLink1;
        newLink1.PointMassA = i*res+j;
        newLink1.PointMassB = i*res+j+1;
        newLink1.restingDistance=1.0f/float(res);

        newLink1.stiffness=0.5f;
        newLink1.damping=0.5f;
        m_springs.push_back(newLink1);
      }
    }
  }

  //--------------BEND SPRINGS----------------------

  for(int i =0 ; i<res; ++i)
  {
      for(int j =0; j<res; ++j)
      {
          if(i<(res-2) && j<(res-2))
          {
            Spring newBend;
            newBend.PointMassA = i*res + j;
            newBend.PointMassB = i*res + j + 2;
            newBend.restingDistance = 2.0f/float(res);
            newBend.stiffness=0.5f;
            newBend.damping=0.5f;
            m_springs.push_back(newBend);
            Spring newBend2;
            newBend2.PointMassA = i*res + j;
            newBend2.PointMassB = (i+2)*res + j;
            newBend2.restingDistance = 2.0f/float(res);
            newBend2.stiffness=0.5f;
            newBend2.damping=0.5f;
            m_springs.push_back(newBend2);
          }
          else if(i>=(res-2) && j<(res-2))
          {
              Spring newBend;
              newBend.PointMassA = i*res + j;
              newBend.PointMassB = i*res + j + 2;
              newBend.restingDistance = 2.0f/float(res);
              newBend.stiffness=0.5f;
              newBend.damping=0.5f;
              m_springs.push_back(newBend);
          }
          else if(j>=(res-2) && i<(res-2))
          {
              Spring newBend2;
              newBend2.PointMassA = i*res + j;
              newBend2.PointMassB = (i+2)*res + j;
              newBend2.restingDistance = 2.0f/float(res);
              newBend2.stiffness=0.5f;
              newBend2.damping=0.5f;
              m_springs.push_back(newBend2);
          }
      }
  }
  //---------------SHEAR SPRINGS-----------------------
  float normald = 1.0f/float(res);
  float distance = sqrt(2*(normald*normald));
  for(int i =0; i<res; ++i)
  {
      for(int j=0; j<res; ++j)
      {
          if(j!=(res-1) && i!=(res-1))
          {
            Spring newLink1;
            newLink1.PointMassA = i*res+j;
            newLink1.PointMassB = (i+1)*res+j+1;
            newLink1.restingDistance=distance;
            newLink1.stiffness=0.5f;
            m_springs.push_back(newLink1);
            Spring newLink2;
            newLink2.PointMassA = i*res+j+1;
            newLink2.PointMassB = (i+1)*res+j;
            newLink2.restingDistance=distance;
            newLink2.stiffness=0.5f;
            m_springs.push_back(newLink2);
          }
      }
  }
}

void ClothScene::updateSimulation(integrators _whichIntegrator)
{
  float deltaT = float(clock()-last_time)/CLOCKS_PER_SEC;
  last_time = clock();

  float timestepLength = 0.016f;
  int timesteps = floor(float(deltaT+leftOvertime)/timestepLength);
  leftOvertime=deltaT-timestepLength*timesteps;

  //std::cout<<"deltaT: "<<deltaT<<"timesteps"<<timesteps<<"\n";
  timesteps=3;
  for(int n =0; n<timesteps; ++n)
  {
    for(int m = 0; m<5; ++m)
    {
      std::fill(m_forces.begin(), m_forces.end(), glm::vec3(0.0f,0.0f,0.0f));
      //------------------------------SPRINGS---------------------------------
      for(int i=0; i<m_springs.size(); ++i)
      {
        int A = m_springs[i].PointMassA;
        int B = m_springs[i].PointMassB;
        if(_whichIntegrator==EULER_FORCES)
        {
          float Kr = m_springs[i].stiffness;
          float Kd = 0.1f;
          glm::vec3 L = vertexPositions[A] - vertexPositions[B];
          float Lnorm = glm::length(L);
          float R = m_springs[i].restingDistance;
          glm::vec3 vA = pointMasses[A].velocity;
          glm::vec3 vB = pointMasses[B].velocity;

          glm::vec3 ourForce = - Kr*(Lnorm - R)*(L/Lnorm) - Kd*(glm::dot(vA-vB,L)/Lnorm)*(L/Lnorm);
          //if(A==0 || B==0) std::cout<<ourForce.x<<","<<ourForce.y<<","<<ourForce.z<<"\n";
          m_forces[A] += ourForce;
          m_forces[B] -= ourForce;
        }
        else
        {
          glm::vec3 differenceXYZ = vertexPositions[A] - vertexPositions[B];
          float d = glm::length(differenceXYZ);

          //-------------------------------------------------------------------

          float im1 = 1.0f/pointMasses[A].mass;
          float im2 = 1.0f/pointMasses[B].mass;

          float scalarP1 = (im1 / (im1 + im2)) * m_springs[i].stiffness;
          float scalarP2 =  m_springs[i].stiffness - scalarP1;

          //-------------------------------------------------------------------

          float differenceScalar = (m_springs[i].restingDistance -d)/d;

          glm::vec3 translationP1 = differenceXYZ*scalarP1*differenceScalar;
          glm::vec3 translationP2 = differenceXYZ*scalarP2*differenceScalar;

          vertexPositions[A] += translationP1;
          vertexPositions[B] -= translationP2;
        }
      }

      //------------------------------ANCHORS---------------------------------

      vertexPositions[(res-1)*res] = glm::vec3(0.0f,(float(res)-1.0f)/float(res),0.0f);
      vertexPositions[(res-1)*res + res -1] = glm::vec3((float(res)-1.0f)/float(res),(float(res)-1.0f)/float(res),0.0f);

    }


    //---------------------------SPHERE COLLISION------------------------------
    for(int i=0; i<res; ++i)
    {
      for(int j=0; j<res; ++j)
      {
        glm::vec3 distance = vertexPositions[i*res + j] - sphereTranslation;
        float d = glm::length(distance);

        float radius = 0.2442f;
        if(d<radius)
        {
          vertexPositions[i*res + j]+=((radius-d)/radius)*distance;
        }
      }
    }

    //--------------------------VERLET/EULER INTEGRATION----------------------------
    for(int i=0; i<res; ++i)
    {
      for(int j=0; j<res; ++j)
      {

        if(_whichIntegrator==VERLET)
        {
          pointMasses[i*res + j].velocity = vertexPositions[i*res + j] - pointMasses[i*res + j].prevPos;
          pointMasses[i*res + j].prevPos = vertexPositions[i*res + j];

          glm::vec3 acceleration = glm::vec3(0.0f,-0.0098f,0.0f);
          //glm::vec3 acceleration = glm::vec3(0.0f,0.0f,0.0f);

          vertexPositions[i*res + j] = vertexPositions[i*res + j] + pointMasses[i*res + j].velocity + acceleration*timestepLength;
        }
        else if(_whichIntegrator==EULER)
        {
          glm::vec3 gravity = glm::vec3(0.0f,-0.0098f,0.0f);
          //glm::vec3 gravity = glm::vec3(0.0f,0.0f,0.0f);
          pointMasses[i*res + j].velocity = pointMasses[i*res + j].velocity + gravity*timestepLength;
          vertexPositions[i*res + j] = vertexPositions[i*res + j] + pointMasses[i*res + j].velocity*timestepLength;
        }
        else if(_whichIntegrator==EULER_FORCES)
        {
          // gravity
          //m_forces[i*res + j] = m_forces[i*res + j] - glm::vec3(0.0f,0.98f,0.0f);

          // acceleration
          glm::vec3 an = m_forces[i*res + j] - glm::vec3(0.0f,0.0098f,0.0f);

          // velocity
          pointMasses[i*res + j].velocity = pointMasses[i*res + j].velocity + an*timestepLength;

          // position
          vertexPositions[i*res + j] = vertexPositions[i*res + j] + pointMasses[i*res + j].velocity*timestepLength;

        }
      }
    }
  }
  moveSphere();
}

void ClothScene::handleKey(int key, int action)
{
  if (action==GLFW_PRESS) {
      switch(key) {
      case GLFW_KEY_W:
        m_sphereDirection=SPHERE_FORWARDS;
        break;
      case GLFW_KEY_S:
        m_sphereDirection=SPHERE_BACKWARDS;
        break;
      case GLFW_KEY_A:
        m_sphereDirection=SPHERE_LEFT;
        break;
      case GLFW_KEY_D:
        m_sphereDirection=SPHERE_RIGHT;
        break;
      case GLFW_KEY_R:
        m_sphereDirection=SPHERE_UP;
        break;
      case GLFW_KEY_F:
        m_sphereDirection=SPHERE_DOWN;
        break;
      }
  }
  if(action==GLFW_RELEASE)
  {
    switch(key) {
    case GLFW_KEY_W:
      m_sphereDirection=STATIONARY;
      break;
    case GLFW_KEY_S:
      m_sphereDirection=STATIONARY;
      break;
    case GLFW_KEY_A:
      m_sphereDirection=STATIONARY;
      break;
    case GLFW_KEY_D:
      m_sphereDirection=STATIONARY;
      break;
    case GLFW_KEY_R:
      m_sphereDirection=STATIONARY;
      break;
    case GLFW_KEY_F:
      m_sphereDirection=STATIONARY;
      break;
    }
    //std::cout<<sphereTranslation.x<<", "<<sphereTranslation.y<<", "<<sphereTranslation.z<<"\n";
  }
}

void ClothScene::moveSphere()
{
  switch(m_sphereDirection) {
  case SPHERE_FORWARDS:
    sphereTranslation+=glm::vec3(0.0f,0.0f,0.01f);
    break;
  case SPHERE_BACKWARDS:
    sphereTranslation+=glm::vec3(0.0f,0.0f,-0.01f);
    break;
  case SPHERE_LEFT:
    sphereTranslation+=glm::vec3(0.01f,0.0f,0.0f);
    break;
  case SPHERE_RIGHT:
    sphereTranslation+=glm::vec3(-0.01f,0.0f,0.0f);
    break;
  case SPHERE_UP:
    sphereTranslation+=glm::vec3(0.0f,0.01f,0.0f);
    break;
  case SPHERE_DOWN:
    sphereTranslation+=glm::vec3(0.0f,-0.01f,0.0f);
    break;
  }
}

void ClothScene::loadMatricesToShader(GLint pid, glm::vec3 translation)
{
  // Our MVP matrices
  glm::mat4 M = glm::mat4(1.0f);
  glm::mat4 MVP, MV;
  glm::mat3 N;

  M[3][0] = translation[0];
  M[3][1] = translation[1];
  M[3][2] = translation[2];


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
}

void ClothScene::updateNormals()
{
  for(int i =0; i<res; ++i)
  {
    for(int j =0 ; j<res; ++j)
    {
      if(i!=0 && j!=0 && i!=(res-1) && j!=(res-1));
      glm::vec3 A = vertexPositions[i*res + j] - vertexPositions[i*res + j +1];
      glm::vec3 B = vertexPositions[i*res + j] - vertexPositions[(i+1)*res + j];
      glm::vec3 C = vertexPositions[i*res + j] - vertexPositions[i*res + j -1];
      glm::vec3 D = vertexPositions[i*res + j] - vertexPositions[(i-1)*res + j];

      glm::vec3 vect1 = glm::cross(B,A);
      glm::vec3 vect2 = glm::cross(D,C);

      vertexNormals[i*res + j]=glm::normalize((vect1+vect2)/2.0f);
    }
  }
}
