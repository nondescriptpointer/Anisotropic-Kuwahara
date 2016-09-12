#include <iostream>
#include <vector>

#include <math.h>
#include <string.h>

// gl libs
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// gliby stuff
#include "ShaderManager.h"
#include "UniformManager.h"
#include "TextureManager.h"

using namespace std;
using namespace gliby;

// TODO:
// kernel generationg
// execute all the drawing passes

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
    SH_IDENTITY,
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

// texture loader
TextureManager* textureManager;

// parameters for the algorithm
int iterations = 8; // 3 to 8
float sigma_t = 2.0; // 0 to 10
float alpha = 1.0; // 1 to 999
int radius = 3; // 1 to 20
float smoothing = 0.33333; // 0 to 100
int q = 8; // 1 to 16

// image properties
int m_width = 512;
int m_height = 512;

// holds all the texture buffer locations
GLuint textures[TEX_MAX];

// framebuffer we render to offscreen
GLuint fbo;

// gauss filter
static void gauss_filter(float *data, int width, int height, float sigma) {
    float twoSigma2 = 2.0 * sigma * sigma;
    int halfWidth = (int)ceil( 2.0 * sigma );

    float *src_data = new float[width * height];
    memcpy(src_data, data, width * height * sizeof(float));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0;
            float w = 0;

            for ( int i = -halfWidth; i <= halfWidth; ++i )	{
                for ( int j = -halfWidth; j <= halfWidth; ++j )	{
                    int xi = x + i;
                    int yj = y + j;
                    if ((xi >= 0) && (xi < width) && (yj >= 0) && (yj < height)) {
                        float r = sqrt((float)(i * i + j * j));
                        float k = exp( -r *r / twoSigma2 );
                        w += k;
                        sum += k * src_data[ xi + yj * width];
                    }
                }
            }

            data[ x + y * width ] = sum / w;
        }
    }

    delete[] src_data;
}

// creation of the kernel sectors
void make_sector(float *krnl, int k, int N, int size, float sigma_r, float sigma_s) {
    float *p = krnl;
    for (int j = 0; j < size; ++j) {
        for (int i = 0; i < size; ++i) {
            float x = i - 0.5 * size + 0.5;
            float y = j - 0.5 * size + 0.5;
            float r = sqrt((double)(x * x + y * y));

            float a = 0.5 * atan2(y, x) / M_PI + k * 1.0 / N;
            if (a > 0.5)
                a -= 1.0;
            if (a < -0.5)
                a += 1.0;

            if ((fabs(a) <= 0.5 / N) && (r < 0.5 * size)) {
                *p = 1;
            } else {
                *p = 0;
            }
            ++p;
        }
    }

    gauss_filter(krnl, size, size, sigma_s);

    p = krnl;
    float mx = 0.0;
    for (int j = 0; j < size; ++j) {
        for (int i = 0; i < size; ++i) {
            float x = i - 0.5 * size + 0.5;
            float y = j - 0.5 * size + 0.5;
            float r = sqrt((double)(x * x + y * y));
            *p *= exp(-0.5 * r * r / sigma_r / sigma_r);
            if (*p > mx) mx = *p;
            ++p;
        }
    }

    p = krnl;
    for (int j = 0; j < size; ++j) {
        for (int i = 0; i < size; ++i) {
            *p /= mx;
            ++p;
        }
    }
}

void updateKernel(){
    // calculate sigma from the kernel size
    const int krnl_size = 32;
    const float sigma = 0.25f * (krnl_size - 1);

    // generate the kernel using make_sector
    float *krnl[4];
    for (int k = 0; k < 4; ++k) {
        krnl[k] = new float[krnl_size * krnl_size];
        make_sector(krnl[k], k, iterations, krnl_size, sigma, smoothing * sigma);
    }

    float *krnlx4 = new float[4 * krnl_size * krnl_size];
    for (int i = 0; i < krnl_size * krnl_size; ++i) {
        for (int k = 0; k < 4; ++k) {
            krnlx4[4*i+k] = krnl[k][i];
        }
    }

    // draw the kernel to the TEX_KRNL texture
    glBindTexture(GL_TEXTURE_2D, textures[TEX_KRNL]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16F_ARB, krnl_size, krnl_size, 0, GL_LUMINANCE, GL_FLOAT, krnl[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // draw the kernel to the TEX_KRNLX4 texture
    glBindTexture(GL_TEXTURE_2D, textures[TEX_KRNLX4]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, krnl_size, krnl_size, 0, GL_RGBA, GL_FLOAT, krnlx4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // draw the kernel texture into the window
	/*
    QImage img(krnl_size, krnl_size, QImage::Format_RGB32);
    for (int j = 0; j < krnl_size; ++j) {
        for (int i = 0; i < krnl_size; ++i) {
            int c = (int)(255.0 * krnl[0][j * krnl_size + i]);
            img.setPixel(i, j, m_jetImage.pixel(c, 0));
        }
    }*/
    // TODO: update
    //mainWindow->kernel->setPixmap(QPixmap::fromImage(img));

    // cleanup
    for (int k = 0; k < 4; ++k) {
        delete krnl[k];
    }
    delete krnlx4;
}

void setupContext(){
    // basic setup
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DITHER);
    glFrontFace(GL_CCW);

    // shaders
    const char* searchPath[] = {"./shaders/"}; 
    shaderManager = new ShaderManager(sizeof(searchPath)/sizeof(char*),searchPath);
    ShaderAttribute attrs[] = {{0,"gl_Vertex"}};
    ShaderAttribute attrs_uv[] = {{0,"vVertex"},{3,"vTexCoord"}};

    // compile the different shaders
    const char* identity_uniforms[] = {"tex"};
    shaders.push_back(shaderManager->buildShaderPair("vertex_uv.glsl","identity.glsl",sizeof(attrs_uv)/sizeof(ShaderAttribute),attrs_uv));
    uniformManagers.push_back(new UniformManager(*shaders.end(),sizeof(identity_uniforms)/sizeof(char*),identity_uniforms));

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

    // setup textures
    const char* texs[] = {"jet.png","test.png"};
    textureManager = new TextureManager();
    textureManager->loadTextures(sizeof(texs)/sizeof(char*),texs,GL_TEXTURE_2D,GL_TEXTURE0);

    // generate our texture buffers
    glEnable(GL_TEXTURE_2D);
    glGenTextures(TEX_MAX, textures);
    for (int i = 0; i < TEX_MAX; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // generate framebuffers
    glGenFramebuffersEXT(1, &fbo);

    // generate kernel
    updateKernel();
}


void render(){
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    //glRectf(-1.0f,-1.0f,1.0f,1.0f);

    // draw to the screen directly
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaders[SH_IDENTITY]);
	glBindTexture(GL_TEXTURE_2D, textures[TEX_KRNL]);
    //glBindTexture(GL_TEXTURE_2D, textureManager->get("test.png"));
    glUniform1i(uniformManagers[SH_IDENTITY]->get("tex"),0);
    glBegin(GL_QUADS);
	glVertexAttrib2f(3, 0, 0);
    glVertex2f(-1, 1);
	glVertexAttrib2f(3, 0, 1);
    glVertex2f(-1, -1);
	glVertexAttrib2f(3, 1, 1);
    glVertex2f(1, -1);
	glVertexAttrib2f(3, 1, 0);
    glVertex2f(1, 1);               
    glEnd();
}

void resizeCallback(GLFWwindow* window, int w, int h){
    glViewport(0,0,w,h);
    /*glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-w/2, w-w/2, -h/2, h-h/2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    std::cout << "Resize" << std::endl;*/
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
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    
    // create window
    window = glfwCreateWindow(800,600,"Kuwahara",NULL,NULL);
    window_w = 800; window_h = 600;
    if(!window){
        std::cerr << "GLFW window creation failed" << std::endl;
        glfwTerminate();
    }
    // make the window's context the current one
    glfwMakeContextCurrent(window);
    // events
    glfwSetWindowSizeCallback(window,resizeCallback);

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
