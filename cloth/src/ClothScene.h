#ifndef ClothScene_H
#define ClothScene_H

// The parent class for this scene
#include <ngl/Obj.h>
#include <GLFW/glfw3.h>
#include <ngl/ShaderLib.h>
#include "scene.h"

enum sphere_directions {STATIONARY, SPHERE_UP, SPHERE_DOWN, SPHERE_LEFT, SPHERE_RIGHT, SPHERE_FORWARDS, SPHERE_BACKWARDS};

enum integrators {VERLET, EULER};

struct PointMass
{
  glm::vec3 velocity;
  glm::vec3 prevPos;
  float mass;
  int index;
};

struct Spring
{
  float restingDistance;
  float stiffness;
  float damping;
  int PointMassA;
  int PointMassB;
};

class ClothScene : public Scene
{
public:
    /// This enumerated type allows the user to flip between the shader methods
    typedef enum {
        SHADER_GOURAUD,
        SHADER_PHONG,
        SHADER_COOKTORRANCE,
        SHADER_TOON
    } ShaderMethod;

    ClothScene();

    /// Called when the scene needs to be painted
    void paintGL() noexcept;

    /// Called when the scene is to be initialised
    void initGL() noexcept;

    /// Allow the user to set the currently active shader method
    void setShaderMethod(ShaderMethod method) {m_shaderMethod = method;}

    void initVertexBuffers();

    void initTriangles();

    void initSpringsAndVerts();

    void updateSimulation(integrators _whichIntegrator);

    void handleKey(int key, int action);

    void loadMatricesToShader(GLint pid, glm::vec3 translation);

    void moveSphere();

    void updateNormals();

private:
    /// Keep track of the currently active shader method
    ShaderMethod m_shaderMethod = SHADER_PHONG;

    GLuint posIdx = 0; // The positional array buffer index - dynamic
    GLuint normalsIdx = 0; // The normal array buffer index - dynamic
    GLuint elementsIdx = 0;
    GLuint vertexArrayIdx;

    bool firsttime = true;

    clock_t last_time;

    float leftOvertime = 0.0f;

    int res = 32;

    std::unique_ptr<ngl::Obj> m_sphere;
    glm::vec3 sphereTranslation = glm::vec3(0.44f,0.33f,0.51f);
    sphere_directions m_sphereDirection;


    std::vector<glm::vec3> vertexPositions;
    std::vector<glm::vec3> vertexNormals;
    std::vector<glm::vec3> m_forces;
    std::vector<PointMass> pointMasses;
    std::vector<Spring> m_springs;


};

#endif // ClothScene_H
