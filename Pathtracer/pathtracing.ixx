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

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	imageStore(target, pos, vec4(1.0, 0.0, 1.0, 1.0));
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