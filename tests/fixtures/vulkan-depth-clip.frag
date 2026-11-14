// SPDX-License-Identifier: GPL-3.0-or-later
#version 450
#ifdef EARLY_TESTS
layout(early_fragment_tests) in;
#endif
layout(push_constant) uniform Params { float z; float depth; } p;
layout(location=0) out vec4 color;
void main() {
    color = vec4(1,0,0,1);
#ifdef EXPLICIT_DEPTH
    gl_FragDepth = p.depth;
#endif
}
