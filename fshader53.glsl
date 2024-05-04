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
in  vec2 texCoord;
// fog
uniform vec4 fogColor;
uniform int fogStyle;
uniform int groundTextureFlag;
uniform float fogStart,fogEnd,fogDensity;
uniform mat4 model_view;
uniform sampler2D texture_2D;
uniform sampler1D texture_1D;
uniform int sphereTextureFlag;
uniform int latticeStyle;
void main() 
{
    float f;
    vec4 textureColor;
    float op = color.w;
    float z = length(model_view * position);
    fColor = color;

    if (groundTextureFlag==1){
        textureColor = texture(texture_2D, texCoord);
        fColor *= textureColor;
    }

    if(sphereTextureFlag==1){
        textureColor = texture(texture_1D, texCoord.x);
        fColor *= textureColor;
    }else if(sphereTextureFlag==2){
        textureColor = texture(texture_2D, texCoord);
        if (textureColor.x<0.2){
        //sphere 1.0 0.84
        //texture 0.0 150/255
        textureColor=vec4(0.9, 0.1, 0.1, 1.0);
        }
         fColor *= textureColor;
    }




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

