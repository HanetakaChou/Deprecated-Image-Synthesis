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

#ifndef _SHADERS_FORWARD_SHADING_PIPELINE_LAYOUT_H_
#define _SHADERS_FORWARD_SHADING_PIPELINE_LAYOUT_H_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct forward_shading_layout_global_set_per_frame_uniform_buffer_binding
{
    DirectX::XMFLOAT4X4 view_transform;
    DirectX::XMFLOAT4X4 projection_transform;
    DirectX::XMFLOAT4 point_light_position_and_radius;
    float point_light_shadow_bias;
    float unused_name_padding_point_light_shadow_bias_1;
    float unused_name_padding_point_light_shadow_bias_2;
    float unused_name_padding_point_light_shadow_bias_3;
};

struct forward_shading_layout_global_set_per_object_uniform_buffer_binding
{
    DirectX::XMFLOAT4X4 model_transform;
};

#elif defined(GL_SPIRV) || defined(VULKAN)

layout(set = 0, binding = 0, column_major) uniform unused_name_global_set_per_frame_uniform_buffer_binding
{
    highp mat4x4 g_view_transform;
    highp mat4x4 g_projection_transform;
    highp vec4 g_point_light_position_and_radius;
    highp float g_point_light_shadow_bias;
    highp float unused_name_padding_point_light_shadow_bias_1;
    highp float unused_name_padding_point_light_shadow_bias_2;
    highp float unused_name_padding_point_light_shadow_bias_3;
};

layout(set = 0, binding = 1, column_major) uniform unused_name_global_set_per_object_uniform_buffer_binding
{
    highp mat4x4 g_model_transform;
};

layout(set = 0, binding = 2) highp uniform sampler2DArray g_point_light_shadow_map;

#else
#error Unknown Compiler
#endif

#endif