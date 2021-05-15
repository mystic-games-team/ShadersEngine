///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    uniform mat4 uWorldMatrix;
    uniform mat4 uWorldViewProjectionMatrix;
}

out vec2 vTexCoord;
out vec4 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main() {
    vTexCoord = aTexCoord;
    vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
    vNormal = vec3(vWorldMatrix * vec4(aNormal, 0.0));
    vViewDir = uCameraPosition - vPosition;
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
} 

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vNormal;
in vec4 vPosition;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oPositions;

void main() {

    oNormals = vec4(normalize(vNormal), 1.0F);
    oAlbedo = texture(uTexture, vTexCoord);
    oPositions = vPosition;
    oColor = texture(uTexture, vTexCoord);
}

#endif
#endif
