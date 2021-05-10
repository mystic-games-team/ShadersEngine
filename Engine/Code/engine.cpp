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
    app->mode = Mode::Mode_Mesh;

    switch (app->mode) {
    case Mode_TexturedQuad: {
        const VertexV3V2 vertices[] =
        {
            { glm::vec3(-0.5, -0.5, 0.0), glm::vec2(0.0, 0.0) },
            { glm::vec3(0.5, -0.5, 0.0), glm::vec2(1.0, 0.0) },
            { glm::vec3(0.5, 0.5, 0.0), glm::vec2(1.0, 1.0) },
            { glm::vec3(-0.5, 0.5, 0.0), glm::vec2(0.0, 1.0) },
        };

        const u16 indices[] =
        {
            0, 1, 2,
            0, 2, 3,
        };

        glGenBuffers(1, &app->embeddedVertices);
        glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &app->embeddedElements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &app->vao);
        glBindVertexArray(app->vao);
        glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
        glBindVertexArray(0);

        app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
        Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
        app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

        app->diceTexIdx = LoadTexture2D(app, "dice.png");
        app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
        app->blackTexIdx = LoadTexture2D(app, "color_black.png");
        app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
        app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
        break; }
    case Mode_Mesh: {

        //VertexBufferLayout vertexBufferLayout = {};
        //vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0,3,0 });
        //vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2,2,3 * sizeof(float) });

        //Submesh submesh = {};
        //submesh.vertexBufferLayout = vertexBufferLayout;
        //submesh.vertices.swap(vertices);
        //submesh.indices.swap(indices);
        //myMesh->submeshes.push_back(submesh);

        app->texturedMeshProgramIdx = LoadProgram(app, "shader2.glsl", "SHOW_TEXTURED_MESH");
        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0, 3 }); // position
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2, 2 }); // texCoord
        app->programUniformTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");
        app->programUniformProjectionMatrix = glGetUniformLocation(texturedMeshProgram.handle, "projectionMatrix");
        app->programUniformCameraMatrix = glGetUniformLocation(texturedMeshProgram.handle, "cameraMatrix");
        app->programUniformModelMatrix = glGetUniformLocation(texturedMeshProgram.handle, "modelMatrix");

        app->patrick = LoadModel(app, "Patrick/Patrick.obj");

        app->entities.emplace_back(vec3(0, 0.0F, 0.0F), vec3(0.2f, 0.2f, 0.2f), app->patrick);
        app->entities.emplace_back(vec3(-1.5F, 0.0F, 0), vec3(0.2f, 0.2f, 0.2f), app->patrick);
        app->entities.emplace_back(vec3(1.5F, 0.0F, 0.0F), vec3(0.2f, 0.2f, 0.2f), app->patrick);
        break; }
    }

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

    glGenFramebuffers(1, &app->frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->frameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachment, 0);

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
    ImGui::Combo("Painted Texture", (int*)&app->currentTextureType, "Albedo Color\0Depth Buffer\0Normals Buffer");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2.5F));
    ImGui::AlignTextToFramePadding();
    ImGui::PopStyleVar();
    if (ImGui::TreeNodeEx("OpenGL Extensions", ImGuiTreeNodeFlags_SpanAvailWidth)) {
        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (int i = 0; i < numExtensions; ++i) {
            ImGui::Text((const char*)glGetStringi(GL_EXTENSIONS, i));
        }
        ImGui::TreePop();
    }

    switch (app->currentTextureType) {
    case TextureTypes::AlbedoColor: {

        break; }
    case TextureTypes::DepthBuffer: {
        ImGui::Image((ImTextureID)app->depthAttachment, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break; }
    case TextureTypes::NormalsBuffer: {

        break; }
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
        app->mainCam->yaw += app->input.mouseDelta.x * app->deltaTime * 10;
        app->mainCam->pitch -= app->input.mouseDelta.y * app->deltaTime * 10;

        if (app->mainCam->pitch > 89.0f)
            app->mainCam->pitch = 89.0f;
        if (app->mainCam->pitch < -89.0f)
            app->mainCam->pitch = -89.0f;
    }

    app->mainCam->RecalcalculateViewMatrix();
}

void Render(App* app)
{
    glBindFramebuffer(GL_FRAMEBUFFER, app->frameBuffer);

    GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    glEnable(GL_DEPTH_TEST);

    switch (app->mode)
    {
        case Mode_TexturedQuad: {
            Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTexturedGeometry.handle);
            glBindVertexArray(app->vao);


            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
            break; }
        case Mode_Mesh: {
            Program& textureMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(textureMeshProgram.handle);

            glUniformMatrix4fv(app->programUniformProjectionMatrix, 1, GL_FALSE, glm::value_ptr(glm::perspective(glm::radians(60.0f), (float)app->displaySize.x / (float)app->displaySize.y, 0.1f, 2000.0f)));
            glUniformMatrix4fv(app->programUniformCameraMatrix, 1, GL_FALSE, glm::value_ptr(app->mainCam->viewMatrix));

            for (auto item = app->entities.begin(); item != app->entities.end(); ++item) {

                Model& model = app->models[(*item).model];
                Mesh& mesh = app->meshes[model.meshIdx];

                glUniformMatrix4fv(app->programUniformModelMatrix, 1, GL_FALSE, glm::value_ptr((*item).mat));

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
            break; }
        default:;
    }
    glBindVertexArray(0);
    glUseProgram(0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, app->frameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
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

