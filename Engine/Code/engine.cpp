//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include "assimp_model_loading.h"
#include "buffer_manager.h"

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void Init(App* app)
{
    app->mode = Mode::Mode_Deferred;

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->cbuffer = CreateBuffer(app->maxUniformBufferSize, GL_UNIFORM_BUFFER, GL_STREAM_DRAW);

    switch (app->mode) {
    case Mode::Mode_Forward: {
        app->texturedMeshProgramIdx = LoadProgram(app, "shader2.glsl", "SHOW_TEXTURED_MESH");
        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0, 3 }); // position
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 1, 3 }); // normals
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2, 2 }); // texCoord
        app->programUniformTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");
        break; }
    case Mode::Mode_Deferred: {
        app->texturedMeshProgramIdx = LoadProgram(app, "shader2.glsl", "DEF_GEOMETRY");
        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0, 3 }); // position
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 1, 3 }); // normals
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2, 2 }); // texCoord
        app->programUniformTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");

        app->lightProgramIdx = LoadProgram(app, "shader2.glsl", "LIGHTING");
        Program& light = app->programs[app->lightProgramIdx];
        light.vertexInputLayout.attributes.push_back({ 0, 3 }); // position
        light.vertexInputLayout.attributes.push_back({ 1, 2 }); // texCoord

        app->gizmosProgramIdx = LoadProgram(app, "shader2.glsl", "GIZMOS");
        Program& gizmos = app->programs[app->gizmosProgramIdx];
        gizmos.vertexInputLayout.attributes.push_back({ 0, 3 }); // position
        break; 
    }
    }

    app->patrick = LoadModel(app, "Patrick/Patrick.obj");

    app->entities.emplace_back(vec3(0, 0.0F, 0.0F), vec3(1, 1, 1), app->patrick);
    app->entities.emplace_back(vec3(-2.5F, 0.0F, 0), vec3(1, 1, 1), app->patrick);
    app->entities.emplace_back(vec3(2.5F, 0.0F, 0.0F), vec3(1, 1, 1), app->patrick);

    app->mainCam = new Camera();

    glGenTextures(1, &app->colorAttachment);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->depthAttachment);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->normalsAttachment);
    glBindTexture(GL_TEXTURE_2D, app->normalsAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->albedoAttachment);
    glBindTexture(GL_TEXTURE_2D, app->albedoAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->positionsAttachment);
    glBindTexture(GL_TEXTURE_2D, app->positionsAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->frameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalsAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->albedoAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->positionsAttachment, 0);

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        switch (framebufferStatus) {
        case GL_FRAMEBUFFER_UNDEFINED: {
            int i = 0;
            break; }
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
            int i = 0;
            break; }
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
            int i = 0;
            break; }
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: {
            int i = 0;
            break; }
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: {
            int i = 0;
            break; }
        case GL_FRAMEBUFFER_UNSUPPORTED: {
            int i = 0;
            break; }
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
            int i = 0;
            break; }
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: {
            int i = 0;
            break; }
        default: {
            int i = 0;
            break; }
        }
    }

    glDrawBuffers(1, &app->colorAttachment);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    app->lights.push_back(Light(LightType::Directional, vec3(1.0F, 0.1f, 0.5f), vec3(-3, -1, -2), vec3(3, 1, 2), 0.5f));
    app->lights.push_back(Light(LightType::Point, vec3(1.0F, 0.3F, 0.3F), vec3(-1, 0, 0), vec3(0, 0, 3), 2));
    app->lights.push_back(Light(LightType::Point, vec3(0, 1, 0), vec3(-1, 0, 0), vec3(-2, 0, 3.5), 4));
    app->lights.push_back(Light(LightType::Directional, vec3(0, 0, 1.0F), vec3(-1, 0, 1), vec3(3, 0, 3), 1));
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
    ImGui::Text("OpenGL Renderer: %s", glGetString(GL_RENDERER));
    ImGui::Text("OpenGL Vendor: %s", glGetString(GL_VENDOR));
    ImGui::Text("OpenGL GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    ImGui::Text("--- Camera Pos ---");
    ImGui::Text("Camera Pos X: %f", app->mainCam->cameraPos.x);
    ImGui::Text("Camera Pos Y: %f", app->mainCam->cameraPos.y);
    ImGui::Text("Camera Pos Z: %f", app->mainCam->cameraPos.z);
    ImGui::Text("------------------");
    ImGui::Combo("Painted Texture", (int*)&app->currentTextureType, "Albedo Color\0Depth Buffer\0Normals Buffer\0Positions");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2.5F));
    ImGui::AlignTextToFramePadding();
    ImGui::PopStyleVar();

    switch (app->currentTextureType) {
    case TextureTypes::AlbedoColor: {
        ImGui::Image((ImTextureID)app->albedoAttachment, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break; }
    case TextureTypes::DepthBuffer: {
        ImGui::Image((ImTextureID)app->depthAttachment, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break; }
    case TextureTypes::NormalsBuffer: {
        ImGui::Image((ImTextureID)app->normalsAttachment, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break; }
    case TextureTypes::PositionBuffer: {
        ImGui::Image((ImTextureID)app->positionsAttachment, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break; }
    }

    if (ImGui::TreeNodeEx("OpenGL Extensions", ImGuiTreeNodeFlags_SpanAvailWidth)) {
        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (int i = 0; i < numExtensions; ++i) {
            ImGui::Text((const char*)glGetStringi(GL_EXTENSIONS, i));
        }
        ImGui::TreePop();
    }

    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    if (app->input.keys[Key::K_W] == ButtonState::BUTTON_PRESSED)
    {
        app->mainCam->cameraPos += app->mainCam->speed * app->mainCam->cameraDirection * app->deltaTime;
    }

    if (app->input.keys[Key::K_S] == ButtonState::BUTTON_PRESSED)
    {
        app->mainCam->cameraPos -= app->mainCam->speed * app->mainCam->cameraDirection * app->deltaTime;
    }

    if (app->input.keys[Key::K_D] == ButtonState::BUTTON_PRESSED)
    {
        app->mainCam->cameraPos -= app->mainCam->speed * app->mainCam->cameraRight * app->deltaTime;
    }

    if (app->input.keys[Key::K_A] == ButtonState::BUTTON_PRESSED)
    {
        app->mainCam->cameraPos += app->mainCam->speed * app->mainCam->cameraRight * app->deltaTime;
    }

    if (app->input.mouseButtons[MouseButton::LEFT] == BUTTON_PRESSED)
    {
        app->mainCam->yaw += app->input.mouseDelta.x * app->deltaTime * 30;
        app->mainCam->pitch -= app->input.mouseDelta.y * app->deltaTime * 30;

        if (app->mainCam->pitch > 89.0f)
            app->mainCam->pitch = 89.0f;
        if (app->mainCam->pitch < -89.0f)
            app->mainCam->pitch = -89.0f;
    }

    app->mainCam->RecalcalculateViewMatrix();
}

void renderQuad()
{
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;

    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Render(App* app)
{
    glBindFramebuffer(GL_FRAMEBUFFER, app->frameBuffer);

    GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    glEnable(GL_DEPTH_TEST);

    switch (app->mode)
    {
    case Mode_Forward: {
        Program& textureMeshProgram = app->programs[app->texturedMeshProgramIdx];
        glUseProgram(textureMeshProgram.handle);

        MapBuffer(app->cbuffer, GL_WRITE_ONLY);

        app->globalParamsOffset = app->cbuffer.head;

        PushVec3(app->cbuffer, app->mainCam->cameraPos);

        PushUInt(app->cbuffer, app->lights.size());

        for (u32 i = 0; i < app->lights.size(); ++i)
        {
            AlignHead(app->cbuffer, sizeof(vec4));

            Light& light = app->lights[i];
            PushUInt(app->cbuffer, light.type);
            PushVec3(app->cbuffer, light.color);
            PushVec3(app->cbuffer, light.direction);
            PushVec3(app->cbuffer, light.position);
            PushFloat(app->cbuffer, light.intensity);
            PushFloat(app->cbuffer, 0.82f);
            PushFloat(app->cbuffer, 1.63f);
        }

        app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

        glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

        for (auto item = app->entities.begin(); item != app->entities.end(); ++item) {
            AlignHead(app->cbuffer, app->uniformBlockAlignment);

            Entity& entity = *item;
            glm::mat4 world = entity.mat;
            glm::mat4 worldViewProjection = glm::perspective(glm::radians(60.0f), (float)app->displaySize.x / (float)app->displaySize.y, 0.1f, 2000.0f) * app->mainCam->viewMatrix * world;


            entity.localParamsOffset = app->cbuffer.head;
            PushMat4(app->cbuffer, world);
            PushMat4(app->cbuffer, worldViewProjection);
            entity.localParamsSize = app->cbuffer.head - entity.localParamsOffset;

            glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

            Model& model = app->models[entity.model];
            Mesh& mesh = app->meshes[model.meshIdx];

            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
            {
                GLuint vao = FindVAO(mesh, i, textureMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshMaterial = app->materials[submeshMaterialIdx];

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                glUniform1i(app->programUniformTexture, 0);

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            }
        }

        UnmapBuffer(app->cbuffer);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, app->frameBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        break; }
    case Mode::Mode_Deferred: {
        Program& textureMeshProgram = app->programs[app->texturedMeshProgramIdx];
        glUseProgram(textureMeshProgram.handle);

        MapBuffer(app->cbuffer, GL_WRITE_ONLY);

        app->globalParamsOffset = app->cbuffer.head;

        app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

        glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

        for (auto item = app->entities.begin(); item != app->entities.end(); ++item) {
            AlignHead(app->cbuffer, app->uniformBlockAlignment);

            Entity& entity = *item;
            glm::mat4 world = entity.mat;
            glm::mat4 worldViewProjection = glm::perspective(glm::radians(60.0f), (float)app->displaySize.x / (float)app->displaySize.y, 0.1f, 2000.0f) * app->mainCam->viewMatrix * world;


            entity.localParamsOffset = app->cbuffer.head;
            PushMat4(app->cbuffer, world);
            PushMat4(app->cbuffer, worldViewProjection);
            entity.localParamsSize = app->cbuffer.head - entity.localParamsOffset;

            glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

            Model& model = app->models[entity.model];
            Mesh& mesh = app->meshes[model.meshIdx];

            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
            {
                GLuint vao = FindVAO(mesh, i, textureMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshMaterial = app->materials[submeshMaterialIdx];

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                glUniform1i(app->programUniformTexture, 0);

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, NULL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(app->programs[app->lightProgramIdx].handle);

        glUniform1i(glGetUniformLocation(app->programs[app->lightProgramIdx].handle, "uPositionTexture"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->positionsAttachment);

        glUniform1i(glGetUniformLocation(app->programs[app->lightProgramIdx].handle, "uNormalsTexture"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, app->normalsAttachment);

        glUniform1i(glGetUniformLocation(app->programs[app->lightProgramIdx].handle, "uAlbedoTexture"), 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, app->albedoAttachment);

        glUniform1i(glGetUniformLocation(app->programs[app->lightProgramIdx].handle, "uDepthTexture"), 3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, app->depthAttachment);

        AlignHead(app->cbuffer, app->uniformBlockAlignment);

        app->globalParamsOffset = app->cbuffer.head;

        PushVec3(app->cbuffer, app->mainCam->cameraPos);
        PushUInt(app->cbuffer, app->lights.size());

        for (u32 i = 0; i < app->lights.size(); ++i)
        {
            AlignHead(app->cbuffer, sizeof(vec4));

            Light& light = app->lights[i];
            PushUInt(app->cbuffer, light.type);
            PushVec3(app->cbuffer, light.color);
            PushVec3(app->cbuffer, light.direction);
            PushVec3(app->cbuffer, light.position);
            PushFloat(app->cbuffer, light.intensity);
            PushFloat(app->cbuffer, 0.82f);
            PushFloat(app->cbuffer, 1.63f);
        }
        app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

        glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

        UnmapBuffer(app->cbuffer);

        renderQuad();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, app->frameBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
        glBlitFramebuffer(
            0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glUseProgram(app->programs[app->gizmosProgramIdx].handle);

        glUniformMatrix4fv(glGetUniformLocation(app->programs[app->gizmosProgramIdx].handle, "projectionView"), 1, GL_FALSE, glm::value_ptr(glm::perspective(glm::radians(60.0f), (float)app->displaySize.x / (float)app->displaySize.y, 0.1f, 2000.0f)* app->mainCam->viewMatrix));
        for (unsigned int i = 0; i < app->lights.size(); ++i) {
            glm::mat4 mat = glm::mat4(1.f);
            mat = glm::translate(mat, app->lights[i].position);
            mat = glm::scale(mat, vec3(0.5f));
            glUniformMatrix4fv(glGetUniformLocation(app->programs[app->gizmosProgramIdx].handle, "model"), 1, GL_FALSE, glm::value_ptr(mat));
            glUniform3fv(glGetUniformLocation(app->programs[app->gizmosProgramIdx].handle, "lightColor"), 1, glm::value_ptr(app->lights[i].color));
            renderSphere();
        }
        break; }
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
        {
            return submesh.vaos[i].handle;
        }
    }

    GLuint vaoHandle = 0;

    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;

                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }

        assert(attributeWasLinked);
    }

    glBindVertexArray(0);

    VAO vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void renderSphere()
{
    static unsigned int sphereVAO = 0;
    static unsigned int indexCount; 

    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        float stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}