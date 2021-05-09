///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=2) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 cameraMatrix;
uniform mat4 modelMatrix;
uniform mat4 projectionMatrix;

void main() {
    vTexCoord = aTexCoord;

    gl_Position = projectionMatrix * cameraMatrix * modelMatrix * vec4(aPosition, 1.0F);
} 

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main() {
    oColor = texture(uTexture, vTexCoord);
}

#endif
#endif
