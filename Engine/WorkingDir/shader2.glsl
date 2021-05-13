///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;

out vec2 vTexCoord;
out vec3 normals;

uniform mat4 cameraMatrix;
uniform mat4 modelMatrix;
uniform mat4 projectionMatrix;

void main() {
    vTexCoord = aTexCoord;
    normals = mat3(modelMatrix) * aNormals;

    gl_Position = projectionMatrix * cameraMatrix * modelMatrix * vec4(aPosition, 1.0F);
} 

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 normals;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;

void main() {

    oNormals = vec4(normalize(normals), 1.0F);
    oColor = texture(uTexture, vTexCoord);
}

#endif
#endif
