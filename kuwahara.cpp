#include <iostream>

// gl libs
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// window reference
GLFWwindow* window;
int window_w, window_h;

void setupContext(){

}

void render(){

}

int main(int argc, char **argv){
    // init glfw 
    if(!glfwInit()){
        std::cerr << "GLFW init failed" << std::endl;
        return -1;
    }

    // swap interval
    glfwSwapInterval(1);
    // set window open hints
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // create window
    window = glfwCreateWindow(800,600,"Kuwahara",NULL,NULL);
    window_w = 800; window_h = 600;
    if(!window){
        std::cerr << "GLFW window creation failed" << std::endl;
        glfwTerminate();
    }
    // make the window's context the current one
    glfwMakeContextCurrent(window);

    // init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(err != GLEW_OK){
        std::cerr << "Glew error: " << glewGetErrorString(err) << std::endl;
    }

    // setup the context
    setupContext();

    // main loop
    while(!glfwWindowShouldClose(window)){
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
