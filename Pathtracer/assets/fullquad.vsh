layout(location = 0) in vec2 pos;
layout(location = 0) out vec2 pos_out;

void main() {
	pos_out = pos * 0.5 + 0.5;

	gl_Position = vec4(pos, 0.5, 1.0);
}