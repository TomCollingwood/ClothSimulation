// Must include our scene first because of GL dependency order
#include "ClothScene.h"

// This will probably already be included by a scene file
#include "glinclude.h"
#include "fixedcamera.h"
#include "trackballcamera.h"

// Includes for GLFW
#include <GLFW/glfw3.h>
ClothScene g_scene;
TrackballCamera g_camera;
GLfloat g_focalDepth = 1.0f;
GLfloat g_blurRadius = 0.01f;

void error_callback(int error, const char* description) {
    std::cerr << "Error ("<<error<<"): " << description << "\n";
}

void cursor_callback(GLFWwindow* /*window*/, double xpos, double ypos) {
    g_camera.handleMouseMove(xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Retrieve the position of the mouse click
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Call the camera to handle the click action
    g_camera.handleMouseClick(xpos, ypos, button, action, mods);
}

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
    // Escape exits the application
    if (action == GLFW_PRESS) {
        switch(key) {
        case GLFW_KEY_ESCAPE: //exit the application
            glfwSetWindowShouldClose(window, true);
        }
        g_scene.handleKey(key,action);
    }
    else if(action ==GLFW_RELEASE)
    {
      g_scene.handleKey(key,action);
    }

    // Any other keypress should be handled by our camera
    g_camera.handleKey(key, action==GLFW_PRESS );
}


void resize_callback(GLFWwindow */*window*/, int width, int height) {
    // Resize event
    //int width, height;
    //glfwGetFramebufferSize(window, &width, &height);
    g_camera.resize(width,height);
    g_scene.resizeGL(width,height);
}

int main() {
    if (!glfwInit()) {
        // Initialisation failed
        glfwTerminate();
    }

    // Register error callback
    glfwSetErrorCallback(error_callback);

		// Set our OpenGL version
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_SAMPLES,8);


    // Create our window in a platform agnostic manner
    int width = 1280; int height = 720;
    GLFWwindow* window = glfwCreateWindow(width, // width
                                          height, // height
                                          "Smile!", // title
                                          nullptr, // monitor for full screen
                                          nullptr); // return value on failure

    if (window == nullptr) {
            // Window or OpenGL context creation failed
            glfwTerminate();
    }
    // Make the window an OpenGL window
    glfwMakeContextCurrent(window);

    // Initialise GLEW - note this generates an "invalid enumerant" error on some platforms
#if ( (!defined(__MACH__)) && (!defined(__APPLE__)) )
    glewExperimental = GL_TRUE;
    glewInit();
    glGetError(); // quietly eat errors from glewInit()
#endif

    // Set keyboard callback
    glfwSetKeyCallback(window, key_callback);

    // Disable the cursor for the FPS camera
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse move and click callback
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Set the lastX,Y position on the FPS camera
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    g_camera.setInitialMousePos(mouseX, mouseY);

    // Needed for the fixed camera
    g_camera.setTarget(0.0f,0.0f,0.0f);
    g_camera.setEye(0.0f,0.0f,-2.0f);

    // Initialise our OpenGL scene
    g_scene.initGL();

    // Set the window resize callback and call it once
    glfwSetFramebufferSizeCallback(window, resize_callback);
    resize_callback(window, width, height);

    g_scene.resizeGL(1280,720);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Poll events
        glfwPollEvents();

        // Update our camera matrices
        g_camera.update();

        // Set the view-projection matrix
        g_scene.setViewMatrix(g_camera.viewMatrix());
        g_scene.setProjMatrix(g_camera.projMatrix());

        // Draw our GL stuff
        g_scene.paintGL();

        // Swap the buffers
        glfwSwapBuffers(window);
    }

    // Close up shop
    glfwDestroyWindow(window);
    glfwTerminate();
}
