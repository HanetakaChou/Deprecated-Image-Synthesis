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

#include "forward_shading_pipeline_layout.h"
#include "mesh_vertex.h"

layout(location = 0) out highp vec3 out_position_world_space;

void main()
{
    // Model Space
    highp vec3 position_model_space = in_position_xyzw.xyz;

    // World Space
    highp vec3 position_world_space = (g_model_transform * vec4(position_model_space, 1.0)).xyz;

    // View Space
    highp vec3 position_view_space = (g_view_transform * vec4(position_world_space, 1.0)).xyz;

    // Clip Space
    highp vec4 position_clip_space = g_projection_transform * vec4(position_view_space, 1.0);

    out_position_world_space = position_world_space;
    gl_Position = position_clip_space;
}
