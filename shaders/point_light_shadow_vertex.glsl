#version 310 es

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

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable

#include "point_light_shadow_pipeline_layout.h"
#include "mesh_vertex.h"

layout(location = 0) out highp float out_inverse_length_view_space;

void main()
{
    // Model Space
    highp vec3 position_model_space = in_position_xyzw.xyz;

    // World Space
    highp vec3 position_world_space = (g_model_transform * vec4(position_model_space, 1.0)).xyz;

    // Light Information
    highp vec3 point_light_position = g_point_light_position_and_radius.xyz;
    highp float point_light_radius = g_point_light_position_and_radius.w;

    // View Space
    highp vec3 position_view_space = position_world_space - point_light_position;

    // Sphere Space
    highp vec3 position_sphere_space = normalize(position_view_space);

    // Clip Space
    highp vec4 position_clip_space;

    // NOTE: We should always draw twice, otherwise rasterization may be NOT correct when the same triangle covers different octants.
    // z < 0 -> layer 0
    // z > 0 -> layer 1
    highp int layer_index = 1;

    // Octahedron Mapping
    highp float manhattan_norm = abs(position_sphere_space.x) + abs(position_sphere_space.y) + abs(position_sphere_space.z);
    position_clip_space.w = manhattan_norm;

    highp vec2 layer_position_clip_space_xy[2] = vec2[2]((manhattan_norm - abs(position_sphere_space.yx)) * vec2((position_sphere_space.x >= 0.0) ? 1.0 : -1.0, (position_sphere_space.y >= 0.0) ? 1.0 : -1.0), position_sphere_space.xy);
    position_clip_space.xy = layer_position_clip_space_xy[layer_index];

    // TODO: Vulkan Viewport Flip y

    // Pseudo Depth (For Cliping and Culling)
    highp float layer_position_clip_space_z[2] = float[2](-position_sphere_space.z, position_sphere_space.z);
    position_clip_space.z = layer_position_clip_space_z[layer_index];

    // Perspective Correct Interpolation
    out_inverse_length_view_space = 1.0 / length(position_view_space);

    gl_Position = position_clip_space;
}
