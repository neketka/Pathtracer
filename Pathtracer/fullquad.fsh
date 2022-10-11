layout(location = 0) out vec4 fragment;
layout(location = 0) in vec2 pos;

layout(location = 0) uniform sampler2D tex;

void main() {
	fragment = vec4(texture(tex, pos).rgb, 1.0);
}