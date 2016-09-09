#include <iostream>
#include <vector>

// gl libs
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// gliby stuff
#include "ShaderManager.h"
#include "UniformManager.h"

using namespace std;
using namespace gliby;

// window reference
GLFWwindow* window;
int window_w, window_h;
// shader management loads and compiles our shaders
ShaderManager* shaderManager;

// enum to keep track of all the different texture buffers
enum {
    TEX_KRNL,
    TEX_KRNLX4,
    TEX_NOISE,
    TEX_SRC,
    TEX_DST,
    TEX_LIC,
    TEX_A,
    TEX_FTM,
    TEX_TMP0,
    TEX_TMP1,
    TEX_MAX
};
// shader tracking
enum {
    SH_AKF_V1,
    SH_AKF_V2,
    SH_GAUSS,
    SH_GKF,
    SH_KUWAHARA,
    SH_LIC,
    SH_SHOWA,
    SH_SST,
    SH_TFM,
    SH_MAX
};
vector<GLuint> shaders;
vector<UniformManager*> uniformManagers;

// parameters for the algorithm
int iterations = 8;

void setupContext(){
    // basic setup
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DITHER);
    glFrontFace(GL_CCW);

    // shaders
    const char* searchPath[] = {"./shaders/"}; 
    shaderManager = new ShaderManager(sizeof(searchPath)/sizeof(char*),searchPath);
    ShaderAttribute attrs[] = {{0,"vVertex"}};

    // compile the different shaders
    const char* akf_v1_uniforms[] = {"src","K0","tfm","radius","q","alpha"}; 
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","akf_v1.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(akf_v1_uniforms)/sizeof(char*),akf_v1_uniforms));

    const char* akf_v2_uniforms[] = {"src","K0123","tfm","radius","q","alpha"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","akf_v2.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(akf_v2_uniforms)/sizeof(char*),akf_v2_uniforms));

    const char* gauss_uniforms[] = {"src","sigma"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","gauss.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(gauss_uniforms)/sizeof(char*),gauss_uniforms));

    const char* gkf_uniforms[] = {"src","K0","radius","q"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","gkf.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(gkf_uniforms)/sizeof(char*),gkf_uniforms));

    const char* kuwahara_uniforms[] = {"src","radius"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","kuwahara.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(kuwahara_uniforms)/sizeof(char*),kuwahara_uniforms));

    const char* lic_uniforms[] = {"tfm","src","sigma","dst"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","lic.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(lic_uniforms)/sizeof(char*),lic_uniforms));

    const char* showa_uniforms[] = {"tfm","jet"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","showa.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(showa_uniforms)/sizeof(char*),showa_uniforms));

    const char* sst_uniforms[] = {"src"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","sst.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(sst_uniforms)/sizeof(char*),sst_uniforms));

    const char* tfm_uniforms[] = {"src"};
    shaders.push_back(shaderManager->buildShaderPair("vertex.glsl","tfm.glsl",sizeof(attrs)/sizeof(ShaderAttribute),attrs));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(tfm_uniforms)/sizeof(char*),tfm_uniforms));
}

void render(){
    glClear(GL_COLOR_BUFFER_BIT);

    
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
