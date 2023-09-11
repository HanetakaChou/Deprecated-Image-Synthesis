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

#include "point_light_shadow_pipeline_layout.h"

layout(location = 0) in highp float in_inverse_length_view_space;

void main()
{
    // Perspective Correct Interpolation
    highp float length_view_space = 1.0 / in_inverse_length_view_space;

    // Light Information
    highp float point_light_radius = g_point_light_position_and_radius.w;

    // Real Depth
    highp float ratio = length_view_space / point_light_radius;

    gl_FragDepth = ratio;
}
