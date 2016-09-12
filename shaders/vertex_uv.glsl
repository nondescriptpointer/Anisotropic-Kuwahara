#version 430
in vec4 vVertex;
in vec2 vTexCoord;

smooth out vec2 vTex;

void main(void){
    vTex = vTexCoord;
    gl_Position = vVertex;
}
