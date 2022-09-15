module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module transforms;

export glm::mat4 cameraViewMatrix(glm::vec3 pos, glm::vec3 rot) {
	return glm::translate(glm::mat4(1.0), pos) *
		glm::rotate(glm::mat4(1.0), rot.x, glm::vec3(1.0, 0.0, 0.0)) *
		glm::rotate(glm::mat4(1.0), rot.y, glm::vec3(0.0, 1.0, 0.0)) *
		glm::rotate(glm::mat4(1.0), rot.z, glm::vec3(0.0, 0.0, 1.0));
}