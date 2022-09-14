module;

#include <glm/glm.hpp>
#include <string>

export module pathtracing;

import texture;
import pass;
import shader;

std::string cPathtracing = R"glsl(

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f) uniform image2D target;

const vec3 frustumTL = vec3(0.577, 0.577, -0.577);
const vec3 frustumBR = vec3(0.577, -0.577, -0.577);
const vec3 frustumOrigin = vec3(0.0, 0.0, 0.0);

const vec3 spherePos = vec3(0.0, 0.0, -10.0);
const float sphereRadius = 2.0;

// Returns true and sets intersection position and normal, or returns false
bool sphereRay(vec3 rayPos, vec3 rayDir, out vec3 pos, out vec3 normal) {
	return false;
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = ivec2(gl_NumWorkGroups.xy);

	vec2 color = vec2(pos) / vec2(size);

	imageStore(target, pos, vec4(color, 0.0, 1.0));
}

)glsl";

export class PathtracingPass {
public:
	PathtracingPass() {
		m_program = new GpuProgram(cPathtracing);
	}

	void render(int w, int h, GpuTexture<glm::vec4> *target) {
		GpuComputePass pass(m_program, std::forward<GpuProgramState>(GpuProgramState().image(0, target)));

		pass.attach();
		pass.dispatch(w, h, 1);
	}
private:
	GpuProgram* m_program;
};