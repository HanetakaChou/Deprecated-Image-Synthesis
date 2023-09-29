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

#include "forward_shading_pipeline_layout.h"

layout(location = 0) in highp vec3 in_position_world_space;

layout(location = 0) out highp vec4 out_scene_color;

void main()
{
   // Light Information
   highp vec3 point_light_position = g_point_light_position_and_radius.xyz;
   highp float point_light_radius = g_point_light_position_and_radius.w;

   // View Space
   highp vec3 position_view_space = in_position_world_space - point_light_position;

   // Sphere Space
   highp vec3 position_sphere_space = normalize(position_view_space);

   // Clip Space
   highp vec2 position_clip_space_xy;
   highp float position_clip_space_w;

   // Layer
   // z < 0 -> layer 0
   // z > 0 -> layer 1
   highp int layer_index = (position_sphere_space.z < 0.0) ? 0 : 1;

   // Dual Paraboloid Mapping

   // To ensure the front facing of the triangle is the same
   highp vec2 layer_position_clip_space_xy[2] = vec2[2](vec2(-position_sphere_space.x, position_sphere_space.y), vec2(-position_sphere_space.x, position_sphere_space.y));
   position_clip_space_xy = layer_position_clip_space_xy[layer_index];

   // z > 0
   // (x_2d, y_2d, 0) + (0, 0,  1) = λ ((x_sph, y_sph, z_sph) + (0, 0,  1))
   // (x_2d, y_2d) = (x_sph, y_sph) / ( z_sph + 1)
   // z < 0
   // (x_2d, y_2d, 0) + (0, 0, -1) = λ ((x_sph, y_sph, z_sph) + (0, 0, -1))
   highp float layer_position_clip_space_w[2] = float[2](-position_sphere_space.z + 1.0, position_sphere_space.z + 1.0);
   position_clip_space_w = layer_position_clip_space_w[layer_index];

   // NDC Space
   highp vec2 position_ndc_space_xy = position_clip_space_xy / position_clip_space_w;

   // UV
   highp vec2 uv = position_ndc_space_xy * vec2(0.5, 0.5) + vec2(0.5, 0.5);

   //
   highp float closest_ratio = texture(g_point_light_shadow_map, vec3(uv, float(layer_index))).r + g_point_light_shadow_bias;

   highp float current_ratio = length(position_view_space) / point_light_radius;

   highp float shadow = (closest_ratio >= current_ratio) ? 1.0 : -1.0;

   out_scene_color = vec4(shadow, shadow, shadow, 1.0);
}