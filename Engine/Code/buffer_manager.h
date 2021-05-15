#pragma once
#include "glm/glm.hpp"
#include "platform.h"

struct Buffer;
typedef unsigned int GLenum;

bool IsPowerOf2(u32 value);

u32 Align(u32 value, u32 alignment);

Buffer CreateBuffer(u32 size, GLenum type, GLenum usage);

void BindBuffer(const Buffer& buffer);

void MapBuffer(Buffer& buffer, GLenum access);

void UnmapBuffer(Buffer& buffer);

void AlignHead(Buffer& buffer, u32 alignment);

void PushAlignedData(Buffer& buffer, const void* data, u32 size, u32 alignment);

#define PushData(buffer, data, size) PushAlignedData(buffer, data, size, 1)
#define PushUInt(buffer, value) { u32 v = value; PushAlignedData(buffer, &v, sizeof(v), 4); }
#define PushFloat(buffer, value) { float v = value; PushAlignedData(buffer, &v, sizeof(v), 4); }
#define PushVec3(buffer, value) PushAlignedData(buffer, glm::value_ptr(value), sizeof(value), sizeof(vec4))
#define PushVec4(buffer, value) PushAlignedData(buffer, glm::value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat3(buffer, value) PushAlignedData(buffer, glm::value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat4(buffer, value) PushAlignedData(buffer, glm::value_ptr(value), sizeof(value), sizeof(vec4))