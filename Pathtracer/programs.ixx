module;

#include <string>

export module programs;

export std::string vSpin = R"glsl(

layout(location = 0) in vec2 pos;
layout(location = 0) out vec2 pos_out;

void main() {
	pos_out = pos;

	gl_Position = vec4(pos, 0.5, 1.0);
}

)glsl";

export std::string fSpin = R"glsl(

layout(location = 0) out vec4 fragment;

layout(location = 0) in vec2 pos;
layout(location = 0) uniform float time;

void main() {
	float sT = time * 0.25;
	vec2 npos = (pos * mat2(cos(sT), -sin(sT), sin(sT), cos(sT)) + vec2(1.0)) * 0.5;
	vec2 t = time * 32.0 * vec2(0.0, 1.0);
	float factor = (sin(npos.x * 128.0 + t.x) + 1.0) * (sin(npos.y * 128.0 + t.y) + 1.0) * 0.25;

	vec3 beadColor = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), sin(sT * 8.0) * 0.5 + 0.5);

	fragment = vec4(mix(vec3(0.1, 0.1, 0.5), beadColor, factor), 1.0);
}

)glsl";

