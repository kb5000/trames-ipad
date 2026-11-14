// SPDX-License-Identifier: GPL-3.0-or-later
#version 450
layout(push_constant) uniform Params { float z; float depth; } p;
void main() {
    vec2 xy[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
    gl_Position = vec4(xy[gl_VertexIndex], p.z, 1);
}
