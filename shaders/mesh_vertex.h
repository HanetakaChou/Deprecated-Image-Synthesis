//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _SHADERS_MESH_VERTEX_H_
#define _SHADERS_MESH_VERTEX_H_ 1

#if defined(__STDC__) || defined(__cplusplus)

// vertex bindings/attributes
// location = 0 binding = 0 position DXGI_FORMAT_R16G16B16A16_FLOAT
struct mesh_vertex_position
{
    uint16_t position_x;
    uint16_t position_y;
    uint16_t position_z;
    uint16_t position_w;
};

static VkVertexInputBindingDescription const mesh_vertex_binding_descriptions[1] = {{0U, sizeof(mesh_vertex_position), VK_VERTEX_INPUT_RATE_VERTEX}};

static VkVertexInputAttributeDescription const mesh_vertex_attribute_descriptions[1] = {{0U, 0U, VK_FORMAT_R16G16B16A16_SFLOAT, 0U}};

#elif defined(GL_SPIRV) || defined(VULKAN)

layout(location = 0) in highp vec4 in_position_xyzw;

#else
#error Unknown Compiler
#endif

#endif