/* 
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
              //      due to different settings of the default GLSL version

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;
out vec4 color;
out vec4 position;
out vec2 texCoord;
uniform vec4 DirectionalAmbientProduct, DirectionalDiffuseProduct, DirectionalSpecularProduct;
uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 model_view;
uniform mat4 projection;
uniform mat3 Normal_Matrix;
uniform vec4 LightPosition;   // Must be in Eye Frame
uniform float Shininess;

uniform float ConstAtt;  // Constant Attenuation
uniform float LinearAtt; // Linear Attenuation
uniform float QuadAtt;   // Quadratic Attenuation
uniform int groundTextureFlag;
uniform vec4 material_ambient;
uniform	vec4 material_diffuse;
uniform	vec4 material_specular;
uniform vec4 default_no_light_color;
uniform vec4 light_direction;
uniform vec4 pos_light_direction;
uniform int light_flag;
uniform int sphereTextureFlag;
uniform int sphereTextureDirection,sphereTextureFrame;
uniform int latticeStyle;
void main()
{
    float cutoffAngle=20.0*3.1415926/180.0;
    position = vPosition;
    vec4 texPosition;


    // texture related
    if(sphereTextureFrame==0){
        texPosition = vPosition;
    }else{
        texPosition = model_view*vPosition;
    }

    if (groundTextureFlag==1){
         texCoord = vTexCoord;
    }else if(sphereTextureFlag==1){

        if (sphereTextureDirection==0){
            texCoord.x = 2.5 * texPosition.x;
        }else{
             texCoord.x = 1.5 * (texPosition.x + texPosition.y + texPosition.z);
        }


    }else if(sphereTextureFlag==2){
         if (sphereTextureDirection==0){
            texCoord.x = 0.5 * (texPosition.x+1);
            texCoord.y = 0.5 * (texPosition.y+1);
         }else{
            texCoord.x = 0.3 * (texPosition.x + texPosition.y + texPosition.z);
            texCoord.y = 0.3 * (texPosition.x - texPosition.y + texPosition.z);
         }
    }




    if (light_flag==0){
        gl_Position = projection * model_view * vPosition;
        color = default_no_light_color;
    }else if(light_flag==1){
        vec3 pos = (model_view * vPosition).xyz;
        vec4 global_ambient_light=vec4(1.0, 1.0, 1.0, 1.0);
        //global light effect

        vec3 L = normalize(-light_direction.xyz);
        vec3 E = normalize( -pos );
        vec3 H = normalize( L + E );
        vec3 N = normalize(model_view * vec4(vNormal, 0.0)).xyz;

        if (dot(N, E) < 0) N = -N;

        float attenuation = 1.0;
        vec4 ambient = DirectionalAmbientProduct;
        float d = max( dot(L, N), 0.0 );
        vec4  diffuse = d * DirectionalDiffuseProduct;

        float s = pow( max(dot(N, H), 0.0), Shininess );
        vec4  specular = s * DirectionalSpecularProduct;

        if( dot(L, N) < 0.0 ) {
        specular = vec4(0.0, 0.0, 0.0, 1.0);
        }
        color = attenuation * (ambient + diffuse + specular);
        //point source
        float lightDist = length(pos - LightPosition.xyz);
        attenuation = 1/(ConstAtt + (LinearAtt*lightDist) + (QuadAtt*lightDist*lightDist));
        L = normalize(LightPosition.xyz - pos);
        H = normalize(L + E);
        ambient = AmbientProduct;
        d = max( dot(L, N), 0.0 );
        diffuse = d * DiffuseProduct;
        s = pow( max(dot(N, H), 0.0), Shininess );
        specular = s * SpecularProduct;
        if( dot(L, N) < 0.0 ) {
        specular = vec4(0.0, 0.0, 0.0, 1.0);
        }
        color += attenuation * (ambient + diffuse + specular);
        color += global_ambient_light* material_ambient;
        gl_Position = projection * model_view * vPosition;


    }else{
        vec3 pos = (model_view * vPosition).xyz;
        vec4 global_ambient_light=vec4(1.0, 1.0, 1.0, 1.0);

        //global light effect
        vec3 L = normalize(-light_direction.xyz);
        vec3 E = normalize( -pos );
        vec3 H = normalize( L + E );
        vec3 N = normalize(model_view * vec4(vNormal, 0.0)).xyz;

        if (dot(N, E) < 0) N = -N;

        float attenuation = 1.0;
        vec4 ambient = DirectionalAmbientProduct;
        float d = max( dot(L, N), 0.0 );
        vec4  diffuse = d * DirectionalDiffuseProduct;

        float s = pow( max(dot(N, H), 0.0), Shininess );
        vec4  specular = s * DirectionalSpecularProduct;

        if( dot(L, N) < 0.0 ) {
        specular = vec4(0.0, 0.0, 0.0, 1.0);
        }
        color = attenuation * (ambient + diffuse + specular);


        //point source
        float lightDist = length(pos - LightPosition.xyz);
        L = normalize(LightPosition.xyz - pos);
        H = normalize(L + E);
        vec3 focusDir= normalize(pos_light_direction.xyz);
        attenuation = 1/(ConstAtt + (LinearAtt*lightDist) + (QuadAtt*lightDist*lightDist));
        // spot light, larger cosine means smaller angle
        float spot_attenuation=0.0f;
        if (dot(focusDir, -L) >= cos(cutoffAngle)) {
        spot_attenuation = pow(dot(focusDir, -L), 15.0);
        }
        attenuation *= spot_attenuation;
        ambient = AmbientProduct;

        d = max( dot(L, N), 0.0 );
        diffuse = d * DiffuseProduct;
        s = pow( max(dot(N, H), 0.0), Shininess );
        specular = s * SpecularProduct;
        if( dot(L, N) < 0.0 ) {
        specular = vec4(0.0, 0.0, 0.0, 1.0);
        }

        color += attenuation * (ambient + diffuse + specular);
        color += global_ambient_light* material_ambient;
        gl_Position = projection * model_view * vPosition;
    }






     }

