//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

enum TextureTypes
{
    AlbedoColor,
    DepthBuffer,
    NormalsBuffer,
    PositionBuffer
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct Buffer {
    GLuint  handle;
    GLenum  type;
    u32     size;
    u32     head;
    void* data;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Entity {
    glm::mat4 mat = glm::mat4(1.0f);
    u32 model;

    u32 localParamsOffset = 0;
    u32 localParamsSize = 0;

    Entity(const glm::vec3& pos, const glm::vec3& scale, u32 model) : mat(glm::translate(pos) * glm::scale(scale)), model(model) {}
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    Mode_Forward,
    Mode_Deferred,
    Mode_Count
};

struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Material 
{
    std::string name;
    vec3 albedo;
    vec3 emissive;
    f32 smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VAO
{
    GLuint handle;
    GLuint programHandle;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32 vertexOffset;
    u32 indexOffset;

    std::vector<VAO> vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;
};

struct Camera
{
    vec3 cameraPos;
    vec3 cameraTarget = glm::vec3(0);
    vec3 cameraDirection;
    vec3 cameraRight;
    vec3 cameraUp;
    glm::mat4 viewMatrix;

    float speed = 5;
    float yaw = -90.0f;
    float pitch = 0;

    Camera()
    {
        cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

        cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraDirection.y = sin(glm::radians(pitch));
        cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraDirection));
        cameraUp = glm::cross(cameraDirection, cameraRight);
        viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraDirection, cameraUp);
    }

    void RecalcalculateViewMatrix()
    {
        cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraDirection.y = sin(glm::radians(pitch));
        cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraDirection = glm::normalize(cameraDirection);

        cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraDirection));
        cameraUp = glm::cross(cameraDirection, cameraRight);

        viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraDirection, cameraUp);
    }
};

enum LightType
{
    Directional,
    Point
};

struct Light
{
    LightType   type;
    vec3        color;
    vec3        direction;
    vec3        position;
    float intensity;

    Light(LightType type, vec3 color, vec3 direction, vec3 position, float intensity) : type(type), color(color), direction(direction), position(position), intensity(intensity) {}
};

struct App
{
    ~App()
    {
        delete mainCam;
    }

    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    TextureTypes currentTextureType = TextureTypes::AlbedoColor;
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Program>  programs;
    std::vector<Material> materials;
    std::vector<Mesh>     meshes;
    std::vector<Model>    models;

    // program indices
    u32 texturedMeshProgramIdx;
    u32 lightProgramIdx;

    // Model
    u32 patrick;

    // Mode
    Mode mode;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    std::vector<Entity> entities;

    Camera* mainCam = nullptr;

    GLuint frameBuffer;
    GLuint colorAttachment;
    GLuint depthAttachment;
    GLuint normalsAttachment;
    GLuint albedoAttachment;
    GLuint positionsAttachment;

    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;

    Buffer cbuffer;
    u32 globalParamsOffset;
    u32 globalParamsSize;

    std::vector<Light> lights;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

