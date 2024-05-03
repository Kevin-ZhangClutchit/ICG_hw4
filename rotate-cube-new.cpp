#include <cstdio>
#include <iostream>
#include "Angel-yjc.h"
#include <string>
#include <fstream>
#include <vector>
#include <tuple>
#include <sstream>
#include <cmath>
#include "texmap.c"
using TRIANGLE = std::vector<std::tuple<float, float, float >>;


typedef Angel::vec3 color3;
typedef Angel::vec3 point3;
typedef Angel::vec4 color4;
typedef Angel::vec4 point4;
constexpr float PI = 3.1415926;
constexpr float R = 1.0;
constexpr float speed = 2.0;
constexpr float d = (speed * 2 * PI * R) / 360;
GLuint Angel::InitShader(const char *vShaderFile, const char *fShaderFile);
GLuint program_light;
GLuint floor_buffer;  /* vertex buffer object id for floor */
GLuint xaxis_buffer;
GLuint yaxis_buffer;
GLuint zaxis_buffer;
GLuint shadow_buffer;
GLuint flat_sphere_buffer;
GLuint smooth_sphere_buffer;
// Projection transformation parameters
GLfloat fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat aspect;       // Viewport aspect ratio
GLfloat zNear = 0.5, zFar = 15.0; //modify zFar to allow the initial point to be seen
vec4 original_vec(0.0,0.0,0.0,1.0);
vec4 VRP(7.0, 3.0, -10.0, 1.0);
vec4 VPN(-7.0, -3.0, 10.0, 0.0);
vec4 VUP(0.0, 1.0, 0.0, 0.0);
mat4 accumulatedRotate = Angel::identity();
std::vector<point3> centers{point3(3.0, 1.0, 5.0), point3(-1.0, 1.0, -4.0), point3(3.5, 1, -2.5)};
int currentCenterIndex = 0;
point3 currentCenter = centers[currentCenterIndex];
GLfloat angle = 0.0; // rotation angle in degrees
vec4 init_eye(VRP); // initial viewer position
vec4 eye = init_eye;               // current viewer position
bool isBegin = false;
bool isRolling = false;
bool isShadow = true;
bool isSolid = true;
bool isLight = false;

enum class Shading : int {
    FlatShading = 1,
    SmoothShading = 2
};

enum class Light : int {
    NoLight = 0,
    PointSource = 1,
    SpotLight = 2
};

enum class Fog: int{
    NoFog = 0,
    Linear = 1,
    Exponential = 2,
    ExponentialSquare = 3
};
enum class TextureGround: int{
    No = 0,
    Yes = 1
};

Shading shadeStyle = Shading::FlatShading;
Light lightStyle = Light::NoLight;
Fog fogStyle = Fog::NoFog;
bool isShadowBlending = true;
TextureGround  groundTextureStyle = TextureGround::No;
static GLuint texName;

const int floor_NumVertices = 6; //(1 face)*(2 triangles/face)*(3 vertices/triangle)
point4 floor_points[floor_NumVertices]; // positions for all vertices
point4 default_null(0.0,0.0,0.0,0.0);
point3 floor_normals[floor_NumVertices];
vec2 floor_texCoord[floor_NumVertices];
int sphere_NumVertices;



point4 xaxis_points[2]={point4(0, 0, 0,1),point4(10, 0, 0,1)};
point4 yaxis_points[2]={point4(0, 0, 0,1),point4(0, 10, 0,1)};
point4 zaxis_points[2]={point4(0, 0, 0,1),point4(0, 0, 10,1)};
vec3 axis_normals[2]={point3(0, 0, 0),point3(0, 0, 0)};

color4 xaxis_color(1,0,0,1);
color4 yaxis_color(1,0,1,1);
color4 zaxis_color(0,0,1,1);

std::vector<TRIANGLE> data;
std::vector<point4> sphere_points;
std::vector<point4> shadow_points;


std::vector<point3> flat_sphere_normals;
std::vector<point3> smooth_sphere_normals;
std::vector<point3> shadow_normals;




point4 floorVertices[4] = {
        point4(5, 0, 8,1),
        point4(5, 0, -4,1),
        point4(-5, 0, -4,1),
        point4(-5, 0, 8,1)
};


// RGBA colors
color3 vertex_colors[8] = {
        color3(0.0, 0.0, 0.0),  // black
        color3(1.0, 0.0, 0.0),  // red
        color3(1.0, 1.0, 0.0),  // yellow
        color3(0.0, 1.0, 0.0),  // green
        color3(0.0, 0.0, 1.0),  // blue
        color3(1.0, 0.0, 1.0),  // magenta
        color3(1.0, 1.0, 1.0),  // white
        color3(0.0, 1.0, 1.0)   // cyan
};

std::vector<point3> translationVectors(3);
std::vector<point3> rotationVectors(3);
point3 OY(0.0, 1.0, 0.0);
//----------------------------------------------------------------------------
typedef Angel::vec4  color4;
typedef Angel::vec4  point4;

/*----- Shader Lighting Parameters -----*/

color4 directional_ambient_light(0.0f,0.0f,0.0f,1.0f);
color4 directional_diffuse_light(0.8f,0.8f,0.8f,1.0f);
color4 directional_specular_light(0.2f,0.2f,0.2f,1.0f);
color4 direction(0.1f,0.0f,-1.0f,0.0f);

point4 light_position(-14.0f, 12.0f, -3.0f, 1.0f );
color4 positional_light_ambient( 0.0f, 0.0f, 0.0f, 1.0f );
color4 positional_light_diffuse( 1.0f, 1.0f, 1.0f, 1.0f );
color4 positional_light_specular( 1.0f, 1.0f, 1.0f, 1.0f );
point4 spot_light_direction = vec4(-6.0f, 0.0f, -4.5f, 1.0f) - light_position;
/*
*/
float const_att = 2.0f;
float linear_att = 0.01f;
float quad_att = 0.001f;

// In World frame.
// Needs to transform it to Eye Frame
// before sending it to the shader(s).


//fog related
color4 fogColor(0.7, 0.7, 0.7, 0.5);
float fogStart=0.0;
float fogEnd=18.0;
float fogDensity=0.09;



mat4 shadowProjectionMat(
        vec4(12.0,14.0,0.0,0.0),
        vec4(0.0,0.0,0.0,0.0),
        vec4(0.0,3.0,12.0,0.0),
        vec4(0.0,-1.0,0.0,12.0)
        ); //ref:http://www.it.hiof.no/~borres/j3d/explain/shadow/p-shadow.html

void setup_fog_effect(){
    glUniform1i(glGetUniformLocation(program_light, "fogStyle"),
                static_cast<int>(fogStyle) );

    glUniform1f(glGetUniformLocation(program_light, "fogStart"),
                fogStart);
    glUniform1f(glGetUniformLocation(program_light, "fogEnd"),
                fogEnd);
    glUniform1f(glGetUniformLocation(program_light, "fogDensity"),
                fogDensity);
    glUniform4fv( glGetUniformLocation(program_light, "fogColor"),
                  1, fogColor );
}
void setup_texture_parameters(){
    glUniform1i(glGetUniformLocation(program_light, "texture_2D"), 0);
}

void setup_sphere_shading(mat4 mv){
    color4 sphere_ambient_color(0.2f,0.2f,0.2f,1.0f);
    color4 sphere_diffuse_color(1.0f,0.84f,0.0f,1.0f);
    color4 sphere_specular_color(1.0f,0.84f,0.0f,1.0f);
    color4 sphere_default_color(1.0f, 0.84f, 0.0f,1.0f);
    color4 directional_ambient_product = directional_ambient_light * sphere_ambient_color;
    color4 directional_diffuse_product = directional_diffuse_light * sphere_diffuse_color;
    color4 directional_specular_product = directional_specular_light * sphere_specular_color;
    color4 ambient_product = positional_light_ambient * sphere_ambient_color;
    color4 diffuse_product = positional_light_diffuse * sphere_diffuse_color;
    color4 specular_product = positional_light_specular * sphere_specular_color;
    float  material_shininess = 125.0f;
    //setup_fog_effect();
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalAmbientProduct"),
                  1, directional_ambient_product );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalDiffuseProduct"),
                  1, directional_diffuse_product );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalSpecularProduct"),
                  1, directional_specular_product );

    glUniform4fv( glGetUniformLocation(program_light, "material_ambient"),
                  1, sphere_ambient_color );
    glUniform4fv( glGetUniformLocation(program_light, "material_diffuse"),
                  1, sphere_diffuse_color );
    glUniform4fv( glGetUniformLocation(program_light, "material_specular"),
                  1, sphere_specular_color );

    glUniform4fv( glGetUniformLocation(program_light, "AmbientProduct"),
                  1, ambient_product );
    glUniform4fv( glGetUniformLocation(program_light, "DiffuseProduct"),
                  1, diffuse_product );
    glUniform4fv( glGetUniformLocation(program_light, "SpecularProduct"),
                  1, specular_product );
    vec4 pos_light_direction = mv * spot_light_direction;
    glUniform4fv( glGetUniformLocation(program_light, "pos_light_direction"),
                  1, pos_light_direction );
    vec4 light_position_mv = mv * light_position;
    glUniform4fv( glGetUniformLocation(program_light, "LightPosition"),
                  1, light_position_mv);
    glUniform1f(glGetUniformLocation(program_light, "ConstAtt"),
                const_att);
    glUniform1f(glGetUniformLocation(program_light, "LinearAtt"),
                linear_att);
    glUniform1f(glGetUniformLocation(program_light, "QuadAtt"),
                quad_att);

    glUniform1f(glGetUniformLocation(program_light, "Shininess"),
                material_shininess );
    glUniform4fv(glGetUniformLocation(program_light, "light_direction"),1,
                 direction );
    glUniform4fv( glGetUniformLocation(program_light, "default_no_light_color"),
                  1, sphere_default_color );

    glUniform1i(glGetUniformLocation(program_light, "light_flag"),
                static_cast<int>(lightStyle) );

    glUniform1i(glGetUniformLocation(program_light, "groundTextureFlag"),
                0 );

}
void setup_sphere_shading(mat4 mv, int manualLightStyle){
    color4 sphere_ambient_color(0.2f,0.2f,0.2f,1.0f);
    color4 sphere_diffuse_color(1.0f,0.84f,0.0f,1.0f);
    color4 sphere_specular_color(1.0f,0.84f,0.0f,1.0f);
    color4 sphere_default_color(1.0f, 0.84f, 0.0f,1.0f);
    color4 directional_ambient_product = directional_ambient_light * sphere_ambient_color;
    color4 directional_diffuse_product = directional_diffuse_light * sphere_diffuse_color;
    color4 directional_specular_product = directional_specular_light * sphere_specular_color;
    color4 ambient_product = positional_light_ambient * sphere_ambient_color;
    color4 diffuse_product = positional_light_diffuse * sphere_diffuse_color;
    color4 specular_product = positional_light_specular * sphere_specular_color;
    float  material_shininess = 125.0f;
    //setup_fog_effect();
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalAmbientProduct"),
                  1, directional_ambient_product );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalDiffuseProduct"),
                  1, directional_diffuse_product );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalSpecularProduct"),
                  1, directional_specular_product );

    glUniform4fv( glGetUniformLocation(program_light, "material_ambient"),
                  1, sphere_ambient_color );
    glUniform4fv( glGetUniformLocation(program_light, "material_diffuse"),
                  1, sphere_diffuse_color );
    glUniform4fv( glGetUniformLocation(program_light, "material_specular"),
                  1, sphere_specular_color );

    glUniform4fv( glGetUniformLocation(program_light, "AmbientProduct"),
                  1, ambient_product );
    glUniform4fv( glGetUniformLocation(program_light, "DiffuseProduct"),
                  1, diffuse_product );
    glUniform4fv( glGetUniformLocation(program_light, "SpecularProduct"),
                  1, specular_product );
    vec4 pos_light_direction = mv * spot_light_direction;
    glUniform4fv( glGetUniformLocation(program_light, "pos_light_direction"),
                  1, pos_light_direction );
    vec4 light_position_mv = mv * light_position;
    glUniform4fv( glGetUniformLocation(program_light, "LightPosition"),
                  1, light_position_mv);
    glUniform1f(glGetUniformLocation(program_light, "ConstAtt"),
                const_att);
    glUniform1f(glGetUniformLocation(program_light, "LinearAtt"),
                linear_att);
    glUniform1f(glGetUniformLocation(program_light, "QuadAtt"),
                quad_att);

    glUniform1f(glGetUniformLocation(program_light, "Shininess"),
                material_shininess );
    glUniform4fv(glGetUniformLocation(program_light, "light_direction"),1,
                 direction );
    glUniform4fv( glGetUniformLocation(program_light, "default_no_light_color"),
                  1, sphere_default_color );

    glUniform1i(glGetUniformLocation(program_light, "light_flag"),
                manualLightStyle );
    glUniform1i(glGetUniformLocation(program_light, "groundTextureFlag"),
                0 );
}


void setup_floor_shading(mat4 mv, int lightFlag){
    color4 floor_diffuse_color(0.0f,1.0f,0.0f,1.0f);
    color4 floor_ambient_color(0.2f,0.2f,0.2f,1.0f);
    color4 floor_specular_color(0.0f,0.0f,0.0f,1.0f);
    color4 floor_default_color( 0.0f, 1.0f, 0.0f, 1.0f);

    color4 directional_ambient_product = directional_ambient_light * floor_ambient_color;
    color4 directional_diffuse_product = directional_diffuse_light * floor_diffuse_color;
    color4 directional_specular_product = directional_specular_light * floor_specular_color;
    color4 ambient_product = positional_light_ambient * floor_ambient_color;
    color4 diffuse_product = positional_light_diffuse * floor_diffuse_color;
    color4 specular_product = positional_light_specular * floor_specular_color;
    float  material_shininess = 125.0f;
    //setup_fog_effect();
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalAmbientProduct"),
                  1, directional_ambient_product );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalDiffuseProduct"),
                  1, directional_diffuse_product );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalSpecularProduct"),
                  1, directional_specular_product );


    glUniform4fv( glGetUniformLocation(program_light, "material_ambient"),
                  1, floor_ambient_color );
    glUniform4fv( glGetUniformLocation(program_light, "material_diffuse"),
                  1, floor_diffuse_color );
    glUniform4fv( glGetUniformLocation(program_light, "material_specular"),
                  1, floor_specular_color );
    glUniform4fv( glGetUniformLocation(program_light, "AmbientProduct"),
                  1, ambient_product );
    glUniform4fv( glGetUniformLocation(program_light, "DiffuseProduct"),
                  1, diffuse_product );
    glUniform4fv( glGetUniformLocation(program_light, "SpecularProduct"),
                  1, specular_product );
    vec4 pos_light_direction = mv * spot_light_direction;
    glUniform4fv( glGetUniformLocation(program_light, "pos_light_direction"),
                  1, pos_light_direction );
    vec4 light_position_mv = mv * light_position;
    glUniform4fv( glGetUniformLocation(program_light, "LightPosition"),
                  1, light_position_mv);
    glUniform1f(glGetUniformLocation(program_light, "ConstAtt"),
                const_att);
    glUniform1f(glGetUniformLocation(program_light, "LinearAtt"),
                linear_att);
    glUniform1f(glGetUniformLocation(program_light, "QuadAtt"),
                quad_att);

    glUniform1f(glGetUniformLocation(program_light, "Shininess"),
                material_shininess );
    glUniform4fv(glGetUniformLocation(program_light, "light_direction"),1,
                direction );
    glUniform4fv( glGetUniformLocation(program_light, "default_no_light_color"),
                  1, floor_default_color );

    glUniform1i(glGetUniformLocation(program_light, "light_flag"),
                lightFlag );
    glUniform1i(glGetUniformLocation(program_light, "groundTextureFlag"),
                static_cast<int>(groundTextureStyle) );
    glUniform1i(glGetUniformLocation(program_light, "texture_2D"), 0);
}

void setup_shadow_shading(mat4 mv){
    color4 shadow_default_color( 0.25f,0.25f,0.25f,0.65f);
    //setup_fog_effect();
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalAmbientProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalDiffuseProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalSpecularProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "material_ambient"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "material_diffuse"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "material_specular"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "AmbientProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "DiffuseProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "SpecularProduct"),
                  1, default_null );
    vec4 pos_light_direction = mv * spot_light_direction;
    glUniform4fv( glGetUniformLocation(program_light, "pos_light_direction"),
                  1, pos_light_direction );
    vec4 light_position_mv = mv * light_position;
    glUniform4fv( glGetUniformLocation(program_light, "LightPosition"),
                  1, light_position_mv);
    glUniform1f(glGetUniformLocation(program_light, "ConstAtt"),
                0.0);
    glUniform1f(glGetUniformLocation(program_light, "LinearAtt"),
                0.0);
    glUniform1f(glGetUniformLocation(program_light, "QuadAtt"),
                0.0);
    glUniform1f(glGetUniformLocation(program_light, "Shininess"),
                0.0 );
    glUniform4fv(glGetUniformLocation(program_light, "light_direction"),1,
                 direction );
    glUniform4fv( glGetUniformLocation(program_light, "default_no_light_color"),
                  1, shadow_default_color );

    glUniform1i(glGetUniformLocation(program_light, "light_flag"),
                0 );
    glUniform1i(glGetUniformLocation(program_light, "groundTextureFlag"),
                0 );
}

void setup_axis_shading(mat4 mv,color4 color){
    //setup_fog_effect();
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalAmbientProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalDiffuseProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "DirectionalSpecularProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "material_ambient"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "material_diffuse"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "material_specular"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "AmbientProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "DiffuseProduct"),
                  1, default_null );
    glUniform4fv( glGetUniformLocation(program_light, "SpecularProduct"),
                  1, default_null );
    vec4 pos_light_direction = mv * spot_light_direction;
    glUniform4fv( glGetUniformLocation(program_light, "pos_light_direction"),
                  1, pos_light_direction );
    vec4 light_position_mv = mv * light_position;
    glUniform4fv( glGetUniformLocation(program_light, "LightPosition"),
                  1, light_position_mv);
    glUniform1f(glGetUniformLocation(program_light, "ConstAtt"),
                0.0);
    glUniform1f(glGetUniformLocation(program_light, "LinearAtt"),
                0.0);
    glUniform1f(glGetUniformLocation(program_light, "QuadAtt"),
                0.0);
    glUniform1f(glGetUniformLocation(program_light, "Shininess"),
                0.0 );
    glUniform4fv(glGetUniformLocation(program_light, "light_direction"),1,
                 direction );
    glUniform4fv( glGetUniformLocation(program_light, "default_no_light_color"),
                  1, color );

    glUniform1i(glGetUniformLocation(program_light, "light_flag"),
                0 );
    glUniform1i(glGetUniformLocation(program_light, "groundTextureFlag"),
                0 );
}


//-------------------------------
// generate 2 triangles: 6 vertices and 6 colors
void floor() {
    vec4 u = floorVertices[3] - floorVertices[0];
    vec4 v = floorVertices[2] - floorVertices[0];
    vec3 normal = normalize(cross(u, v));

    for (int i = 0; i < floor_NumVertices; ++i) {
        floor_normals[i] = normal;
    }
    floor_points[0] = floorVertices[0];
    floor_points[1] = floorVertices[1];
    floor_points[2] = floorVertices[2];
    floor_points[3] = floorVertices[0];
    floor_points[4] = floorVertices[2];
    floor_points[5] = floorVertices[3];
    //8*8 to 10*12 and follow the order of points
    floor_texCoord[0] = vec2(0.0, 0.0);
    floor_texCoord[1] = vec2(0.0, 1.5);
    floor_texCoord[2] = vec2(1.25, 1.5);
    floor_texCoord[3] = vec2(0.0, 0.0);
    floor_texCoord[4] = vec2(1.25, 1.5);
    floor_texCoord[5] = vec2(1.25, 0.0);

}


point3 crossProduct(const point3 &AB) {
    float rx = OY.y * AB.z - OY.z * AB.y;
    float ry = OY.z * AB.x - OY.x * AB.z;
    float rz = OY.x * AB.y - OY.y * AB.x;
    return point3(rx, ry, rz);
}

point3 transMaker(const point3 &point1, const point3 &point2) {
    float dx = point2.x - point1.x;
    float dy = point2.y - point1.y;
    float dz = point2.z - point1.z;

    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (distance != 0.0f) {
        dx /= distance;
        dy /= distance;
        dz /= distance;
    }

    return point3(dx, dy, dz);
}

void initTranslationVectors() {
    point3 A = centers[0];
    point3 B = centers[1];
    point3 C = centers[2];
    translationVectors[0] = transMaker(A, B);
    translationVectors[1] = transMaker(B, C);
    translationVectors[2] = transMaker(C, A);
}

void initRotationVectors() {
    for (int i = 0; i < translationVectors.size(); ++i) {
        rotationVectors[i] = crossProduct(translationVectors[i]);
    }
}
float distance(const point3 &point1, const point3 &point2) {
    float dx = point2.x - point1.x;
    float dy = point2.y - point1.y;
    float dz = point2.z - point1.z;

    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    return distance;
}

bool isEnd() {
    point3 from = centers[currentCenterIndex];
    int next = currentCenterIndex == 2 ? 0 : currentCenterIndex + 1;
    point3 to = centers[next];
    float da = distance(from, to);
    float db = distance(from, currentCenter);
    return db > da;
}

//----------------------------------------------------------------------------
// OpenGL initialization
void init() {
    program_light= InitShader("vshader53.glsl","fshader53.glsl");
    initTranslationVectors();
    initRotationVectors();


    image_set_up();

    /*--- Create and Initialize a texture object ---*/
    glGenTextures(1, &texName);      // Generate texture obj name(s)

    glActiveTexture( GL_TEXTURE0 );  // Set the active texture unit to be 0
    glBindTexture(GL_TEXTURE_2D, texName); // Bind the texture to this texture unit

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImageWidth, ImageHeight,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, Image);




    floor();
    // Create and initialize a vertex buffer object for floor, to be used in display()
    glGenBuffers(1, &floor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points) + sizeof(floor_normals) + sizeof(floor_texCoord),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points), floor_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points), sizeof(floor_normals),
                    floor_normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points) + sizeof(floor_normals),
                    sizeof(floor_texCoord), floor_texCoord);

    glGenBuffers(1, &xaxis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, xaxis_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(xaxis_points) + sizeof(axis_normals),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(xaxis_points), xaxis_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(xaxis_points), sizeof(axis_normals),
                    axis_normals);

    glGenBuffers(1, &yaxis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, yaxis_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(yaxis_points) + sizeof(axis_normals),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(yaxis_points), yaxis_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(yaxis_points), sizeof(axis_normals),
                    axis_normals);

    glGenBuffers(1, &zaxis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, zaxis_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(zaxis_points) + sizeof(axis_normals),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(zaxis_points), zaxis_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(zaxis_points), sizeof(axis_normals),
                    axis_normals);

    glGenBuffers(1, &flat_sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, flat_sphere_buffer);
    glBufferData(GL_ARRAY_BUFFER, sphere_points.size() * sizeof(point4) +
                                  flat_sphere_normals.size() * sizeof(color3),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_points.size() * sizeof(point4), sphere_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sphere_points.size() * sizeof(point4), flat_sphere_normals.size() * sizeof(color3),
                    flat_sphere_normals.data());

    glGenBuffers(1, &smooth_sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, smooth_sphere_buffer);
    glBufferData(GL_ARRAY_BUFFER, sphere_points.size() * sizeof(point4) +
                         smooth_sphere_normals.size() * sizeof(color3),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_points.size() * sizeof(point4), sphere_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sphere_points.size() * sizeof(point4), smooth_sphere_normals.size() * sizeof(color3),
                    smooth_sphere_normals.data());




    glGenBuffers(1, &shadow_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, shadow_buffer);
    glBufferData(GL_ARRAY_BUFFER, shadow_points.size() * sizeof(point4) +
                         shadow_normals.size() * sizeof(point3),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, shadow_points.size() * sizeof(point4),shadow_points.data());
    glBufferSubData(GL_ARRAY_BUFFER,shadow_points.size() * sizeof(point4), shadow_normals.size() * sizeof(point3),
                    shadow_normals.data());
    // Load shaders and create a shader program_light (to be used in display())
    //program_light = InitShader("vshader42.glsl", "fshader42.glsl");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.529, 0.807, 0.92, 0.0);
    glLineWidth(2.0);
}

//----------------------------------------------------------------------------
// drawObj(buffer, num_vertices):
//   draw the object that is associated with the vertex buffer object "buffer"
//   and has "num_vertices" vertices.
//

void drawObjwithShader(GLuint buffer, int num_vertices, GLuint shader){
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(shader, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation( shader, "vNormal" );
    glEnableVertexAttribArray( vNormal );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(point4) * num_vertices) );
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vNormal);
}

void drawObjwithShaderAndTexture(GLuint buffer, int num_vertices, GLuint shader){
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(shader, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation( shader, "vNormal" );
    glEnableVertexAttribArray( vNormal );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(point4) * num_vertices) );
    GLuint vTexCoord = glGetAttribLocation( shader, "vTexCoord" );
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET((sizeof(point4) * num_vertices) + (sizeof(vec3) * num_vertices)) );
    
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vNormal);
    glDisableVertexAttribArray(vTexCoord);
}
void drawObjLinewithShader(GLuint buffer, int num_vertices, GLuint shader) {
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(shader, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation( shader, "vNormal" );
    glEnableVertexAttribArray( vNormal );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(point4) * num_vertices) );
    glDrawArrays(GL_LINES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vNormal);
}

//----------------------------------------------------------------------------
void display(void) {
    GLuint model_view;  // model-view matrix uniform shader variable location
    GLuint projection;  // projection matrix uniform shader variable location
    mat3 normal_matrix;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program_light); // Use the shader program_light

    model_view = glGetUniformLocation(program_light, "model_view");
    projection = glGetUniformLocation(program_light, "projection");
    setup_fog_effect();

/*---  Set up and pass on Projection matrix to the shader ---*/
    mat4 p = Perspective(fovy, aspect, zNear, zFar);
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

/*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by keyboard()
    vec4 at = VPN + VRP;
    vec4 up = VUP;
    mat4 mv = LookAt(eye, at, up);


/*----- Set up the Mode-View matrix for the floor -----*/
    // The set-up below gives a new scene (scene 2), using Correct LookAt() function
    //mv = LookAt(eye, at, up) * Translate(0.3f, 0.0f, 0.0f) * Scale(1.2f, 1.2f, 1.2f);
    //mv = LookAt(eye, at, up) * Translate(3.0, 1.0, 5.0) * Scale(1.0f, 1.0f, 1.0f);
    //
    // The set-up below gives the original scene (scene 1), using Correct LookAt()
    //    mv = Translate(0.0f, 0.0f, 0.3f) * LookAt(eye, at, up) * Scale (1.6f, 1.5f, 3.3f);
    //
    // The set-up below gives the original scene (scene 1), when using previously
    //       Incorrect LookAt() (= Translate(1.0f, 1.0f, 0.0f) * correct LookAt() )
    //    mv = Translate(-1.0f, -1.0f, 0.3f) * LookAt(eye, at, up) * Scale (1.6f, 1.5f, 3.3f);
    //




    //axis
    mv = LookAt(eye, at, up);
    setup_axis_shading(mv,xaxis_color);
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
    drawObjLinewithShader(xaxis_buffer, 2, program_light);
    setup_axis_shading(mv,yaxis_color);
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
    drawObjLinewithShader(yaxis_buffer, 2, program_light);
    setup_axis_shading(mv,zaxis_color);
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
    drawObjLinewithShader(zaxis_buffer, 2, program_light);

    if (isShadow && eye.y >= 0){
        mv = LookAt(eye, at, up);
        model_view = glGetUniformLocation(program_light, "model_view");
        projection = glGetUniformLocation(program_light, "projection");
        glDepthMask(GL_FALSE);

        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
        normal_matrix = NormalMatrix(model_view, 1);
        glUniformMatrix3fv(glGetUniformLocation(program_light, "Normal_Matrix"),
                           1, GL_TRUE, normal_matrix );
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        setup_floor_shading(mv,static_cast<int>(lightStyle));
        drawObjwithShaderAndTexture(floor_buffer, floor_NumVertices,program_light);  // draw the floor

        if(isShadowBlending){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }else{
            glDepthMask(GL_TRUE);
        }


        //shadow
        mv = LookAt(eye, at, up);
        setup_shadow_shading(mv);
        mv = LookAt(eye, at, up) * shadowProjectionMat *
             Translate(currentCenter.x, currentCenter.y, currentCenter.z)
             * accumulatedRotate;
        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
        if (isSolid){
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }else{
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        drawObjwithShader(shadow_buffer, sphere_NumVertices,program_light);

        if(isShadowBlending){
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        mv = LookAt(eye, at, up);
        glUniformMatrix4fv(model_view, 1, GL_TRUE, LookAt(eye, at, up)); // GL_TRUE: matrix is row-major
        normal_matrix = NormalMatrix(model_view, 1);

        glUniformMatrix3fv(glGetUniformLocation(program_light, "Normal_Matrix"),
                           1, GL_TRUE, normal_matrix );
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        setup_floor_shading(mv,static_cast<int>(lightStyle));
        drawObjwithShaderAndTexture(floor_buffer, floor_NumVertices,program_light);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


/*
        glUniformMatrix4fv(model_view, 1, GL_TRUE, LookAt(eye, at, up)); // GL_TRUE: matrix is row-major
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawObjLine(axis_buffer, axis_NumVertices);
*/

    }else{

        model_view = glGetUniformLocation(program_light, "model_view");
        projection = glGetUniformLocation(program_light, "projection");
        setup_floor_shading(mv,static_cast<int>(lightStyle));
        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
        normal_matrix = NormalMatrix(model_view, 1);
        glUniformMatrix3fv(glGetUniformLocation(program_light, "Normal_Matrix"),
                           1, GL_TRUE, normal_matrix );
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawObjwithShaderAndTexture(floor_buffer, floor_NumVertices, program_light);


    }



    //sphere
    if (isSolid){
        GLuint curSpherebuffer;
        if (shadeStyle==Shading::FlatShading){
            curSpherebuffer=flat_sphere_buffer;
        }else{
            curSpherebuffer=smooth_sphere_buffer;
        }
        model_view = glGetUniformLocation(program_light, "model_view");
        projection = glGetUniformLocation(program_light, "projection");
        mv = LookAt(eye, at, up);
        setup_sphere_shading(mv);
        mv = LookAt(eye, at, up) * Translate(currentCenter.x, currentCenter.y, currentCenter.z)
             * accumulatedRotate;
        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);

        normal_matrix = NormalMatrix(model_view, 1);
        glUniformMatrix3fv(glGetUniformLocation(program_light, "Normal_Matrix"),
                           1, GL_TRUE, normal_matrix );
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawObjwithShader(curSpherebuffer, sphere_NumVertices,program_light);
    }else{
        model_view = glGetUniformLocation(program_light, "model_view");
        projection = glGetUniformLocation(program_light, "projection");
        mv = LookAt(eye, at, up);
        setup_sphere_shading(mv,0);
        mv = LookAt(eye, at, up) * Translate(currentCenter.x, currentCenter.y, currentCenter.z)
             * accumulatedRotate;
        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major




        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawObjwithShader(smooth_sphere_buffer, sphere_NumVertices,program_light);


    }



    glutSwapBuffers();

}


//---------------------------------------------------------------------------
void idle(void) {
    if(isRolling&&isBegin){
        accumulatedRotate = Rotate(speed,
                                   rotationVectors[currentCenterIndex].x,
                                   rotationVectors[currentCenterIndex].y,
                                   rotationVectors[currentCenterIndex].z) * accumulatedRotate;
        currentCenter.x = currentCenter.x + translationVectors[currentCenterIndex].x * d;
        currentCenter.y = currentCenter.y + translationVectors[currentCenterIndex].y * d;
        currentCenter.z = currentCenter.z + translationVectors[currentCenterIndex].z * d;
        if (isEnd()) {
            currentCenterIndex = currentCenterIndex == 2 ? 0 : currentCenterIndex + 1;
            currentCenter = centers[currentCenterIndex];
        }
        glutPostRedisplay();
    }

}
void myMouse(int button, int state, int x, int y){
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP && isBegin) {
        isRolling=!isRolling;
    }
    if (isRolling) {
        glutIdleFunc(idle);
    }
    else {
        glutIdleFunc(NULL);
    }
}
//----------------------------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 033: // Escape Key
        case 'q':
        case 'Q':
            exit(EXIT_SUCCESS);
            break;

        case 'X':
            eye[0] += 1.0;
            break;
        case 'x':
            eye[0] -= 1.0;
            break;
        case 'Y':
            eye[1] += 1.0;
            break;
        case 'y':
            eye[1] -= 1.0;
            break;
        case 'Z':
            eye[2] += 1.0;
            break;
        case 'z':
            eye[2] -= 1.0;
            break;

        case 'b':
        case 'B': // Toggle between animation and non-animation
            isBegin = true;
            isRolling = true;
            if (isBegin) glutIdleFunc(idle);
            else glutIdleFunc(NULL);
            break;



        case ' ':  // reset to initial viewer/eye position
            eye = init_eye;
            break;
    }
    glutPostRedisplay();
}

//----------------------------------------------------------------------------
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    aspect = (GLfloat) width / (GLfloat) height;
    glutPostRedisplay();
}

void parseFile() {
    std::cout << "Please input the filename:" << std::endl;
    std::string filename;
    std::cin >> filename;
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Invalid file!" << std::endl;
        exit(-1);
    }
    std::string lineCnt;

    std::getline(infile, lineCnt);
    std::string line;
    int tmpCnt = 0;
    TRIANGLE tri;
    while (std::getline(infile, line)) {

        float x, y, z;
        if (tmpCnt == 0) {
            tmpCnt++;
            continue;
        } else if (tmpCnt == 3) {
            tmpCnt = 0;
            std::stringstream ss(line);
            ss >> x >> y >> z;
            point4 curPoint(x,y,z,1.0);
            sphere_points.emplace_back(curPoint);
            shadow_points.emplace_back(curPoint);
            shadow_normals.emplace_back(vec3(0.0f, 0.0f, 0.0f));
            vec4 normal_smooth = normalize(curPoint-original_vec);
            smooth_sphere_normals.emplace_back(vec3(normal_smooth.x,normal_smooth.y,normal_smooth.z));
            tri.emplace_back(std::make_tuple(x, y, z));

            std::tuple<float, float, float> a_value = tri[0];
            vec4 a{std::get<0>(a_value),std::get<1>(a_value),std::get<2>(a_value),1.0};
            std::tuple<float, float, float> b_value = tri[1];
            vec4 b{std::get<0>(b_value),std::get<1>(b_value),std::get<2>(b_value),1.0};
            std::tuple<float, float, float> c_value = tri[2];
            vec4 c{std::get<0>(c_value),std::get<1>(c_value),std::get<2>(c_value),1.0};

            vec4 u = c - a;
            vec4 v = b - a;
            vec4 flat_normal=normalize(cross(u,v));
            for (int i = 0; i < 3; ++i) {
                flat_sphere_normals.emplace_back(vec3(flat_normal.x,flat_normal.y,flat_normal.z));
            }

            data.emplace_back(tri);
            tri.clear();
        } else {
            std::stringstream ss(line);
            ss >> x >> y >> z;
            point4 curPoint(x,y,z,1.0);
            sphere_points.emplace_back(curPoint);
            shadow_points.emplace_back(curPoint);

            shadow_normals.emplace_back(vec3(0.0f, 0.0f, 0.0f));
            vec4 normal_smooth = normalize(curPoint-original_vec);
            smooth_sphere_normals.emplace_back(vec3(normal_smooth.x,normal_smooth.y,normal_smooth.z));
            tri.emplace_back(std::make_tuple(x, y, z));
            tmpCnt++;
        }


    }

    sphere_NumVertices = 3 * data.size();
}
void myShadowMenu(int id) {
    isShadow = (id==2);
    glutPostRedisplay();
};

void myLightMenu(int id) {
    isLight = (id==2);
    if (!isLight){
        lightStyle=Light::NoLight;
    }else{
        lightStyle=Light::PointSource;
    }
    glutPostRedisplay();
};

void myLightStyleMenu(int id) {
    switch (id) {
        case (1) : {
            lightStyle=Light::SpotLight;
            isLight=true;
            break;
        }
        case (2) : {
            lightStyle=Light::PointSource;
            isLight=true;
            break;
        }
    }
    glutPostRedisplay();
};

void myShadingMenu(int id) {
    switch (id) {
        case (1) : {
            shadeStyle=Shading::FlatShading;
            isSolid=true;
            break;
        }
        case (2) : {
            shadeStyle=Shading::SmoothShading;
            isSolid=true;
            break;
        }
    }
    glutPostRedisplay();
};
void myFogMenu(int id) {
    switch (id) {
        case (1) : {
            fogStyle=Fog::NoFog;
            break;
        }
        case (2) : {
            fogStyle=Fog::Linear;
            break;
        }
        case (3) : {
            fogStyle=Fog::Exponential;
            break;
        }
        case (4) : {
            fogStyle=Fog::ExponentialSquare;
            break;
        }

    }
    glutPostRedisplay();
}

void myBlendingShadowMenu(int id){
    switch (id) {
        case (1) : {
            isShadowBlending = false;
            break;
        }
        case (2) : {
            isShadowBlending = true;
            break;
        }
    }
    glutPostRedisplay();
}

void myTextureGroundMenu(int id){
    switch (id) {
        case (1) : {
            groundTextureStyle = TextureGround::No;
            break;
        }
        case (2) : {
            groundTextureStyle = TextureGround::Yes;
            break;
        }
    }
    glutPostRedisplay();
}
void myMenu(int id){
    switch (id) {
        case (1) :
        {
            eye=init_eye;
            break;
        }
        case (2) :
        {
            exit(EXIT_SUCCESS);
            break;
        }
        case (3) :
        {
            isSolid=!isSolid;
            break;
        }
    }
    glutPostRedisplay();
}


void initMenu(){
    int shadowMenuID = glutCreateMenu(myShadowMenu);
    glutSetMenuFont(shadowMenuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);

    int lightMenuID = glutCreateMenu(myLightMenu);
    glutSetMenuFont(shadowMenuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);

    int lightStyleMenuID = glutCreateMenu(myLightStyleMenu);
    glutSetMenuFont(shadowMenuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Spot Light ", 1);
    glutAddMenuEntry(" Point Source ", 2);

    int shadingMenuID = glutCreateMenu(myShadingMenu);
    glutSetMenuFont(shadingMenuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Flat Shading ", 1);
    glutAddMenuEntry(" Smooth Shading ", 2);

    int fogMenuID = glutCreateMenu(myFogMenu);
    glutSetMenuFont(fogMenuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" No Fog ", 1);
    glutAddMenuEntry(" Linear ", 2);
    glutAddMenuEntry(" Exponential ", 3);
    glutAddMenuEntry(" Exponential Square ", 4);

    int shadowBlendingMenuID = glutCreateMenu(myBlendingShadowMenu);
    glutSetMenuFont(shadowBlendingMenuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);

    int groundTextureMenuID = glutCreateMenu(myTextureGroundMenu);
    glutSetMenuFont(groundTextureMenuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);

    int menuID = glutCreateMenu(myMenu);
    glutSetMenuFont(menuID,GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Default View Point ",1);
    glutAddMenuEntry(" Quit ",2);
    glutAddMenuEntry(" Wire Frame Sphere ",3);
    glutAddSubMenu(" Shadow ", shadowMenuID);
    glutAddSubMenu(" Enable Lighting ", lightMenuID);
    glutAddSubMenu(" Light Source ", lightStyleMenuID);
    glutAddSubMenu(" Shading ", shadingMenuID);
    glutAddSubMenu(" Fog Options ", fogMenuID);
    glutAddSubMenu(" Blending Shadow ", shadowBlendingMenuID);
    glutAddSubMenu(" Texture Mapped Ground ", groundTextureMenuID);
    glutAttachMenu(GLUT_LEFT_BUTTON);
}
int main(int argc, char **argv) {

    parseFile();
    glutInit(&argc, argv);
#ifdef __APPLE__ // Enable core profile of OpenGL 3.2 on macOS.
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowSize(512, 512);
    glutCreateWindow("Color Cube");

#ifdef __APPLE__ // on macOS
    // Core profile requires to create a Vertex Array Object (VAO).
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#else           // on Linux or Windows, we still need glew
    /* Call glewInit() and error checking */
    int err = glewInit();
    if (GLEW_OK != err) {
        printf("Error: glewInit failed: %s\n", (char *) glewGetErrorString(err));
        exit(1);
    }
#endif

    // Get info of GPU and supported OpenGL version
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(NULL);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(myMouse);
    init();
    initMenu();
    glutMainLoop();


    return 0;
}
