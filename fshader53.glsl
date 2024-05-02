/* 
File Name: "fshader53.glsl":
           Fragment Shader
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
              //      due to different settings of the default GLSL version

in  vec4 color;
in vec4 position;
out vec4 fColor;
in float distance;
// fog
uniform vec4 fogColor;
uniform int fogStyle;
uniform float fogStart,fogEnd,fogDensity;
uniform mat4 model_view;

void main() 
{
    float f;
    float op = color.w;
    float z = length(model_view * position);
    fColor = color;
    if (fogStyle==0){
        f=1.0;
    }else if(fogStyle==1){
        f = (fogEnd - z) / (fogEnd - fogStart);
        f = clamp(f, 0, 1);
    }else if(fogStyle==2){
        f = exp(-fogDensity*z);
        f = clamp(f, 0, 1);
    }else if(fogStyle==3){
        f = exp(-fogDensity*z*fogDensity*z);
        f = clamp(f, 0, 1);
    }
    fColor = mix(fogColor, fColor, f);
    fColor.w = op;
} 

