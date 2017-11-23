#ifndef ClothScene_H
#define ClothScene_H

// The parent class for this scene
#include <ngl/Obj.h>
#include <GLFW/glfw3.h>
#include <ngl/ShaderLib.h>
#include "scene.h"

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

    void updateSimulation(float timestep);

private:
    /// Keep track of the currently active shader method
    ShaderMethod m_shaderMethod = SHADER_PHONG;

    GLuint posIdx = 0; // The positional array buffer index - dynamic
    GLuint normalsIdx = 0; // The normal array buffer index - dynamic
    GLuint elementsIdx = 0;
    GLuint vertexArrayIdx;

    int res = 32;

    std::vector<glm::vec3> vertexPositions;
    std::vector<glm::vec3> vertexNormals;
};

#endif // ClothScene_H
