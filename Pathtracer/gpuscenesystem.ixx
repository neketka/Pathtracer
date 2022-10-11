module;

#include <typeinfo>
#include <glm/glm.hpp>

export module gpuscenesystem;

import extengine;
import buffer;
import assetsystem;

export struct GpuTri {
	glm::vec4 pos0uv0x;
	glm::vec4 pos1uv0y;
	glm::vec4 pos2uv1x;

	glm::vec4 normal0uv1y;
	glm::vec4 normal1uv2x;
	glm::vec4 normal2uv2y;

	glm::vec4 materialId;
	glm::vec4 padding0;
};

export struct GpuMat {
	glm::vec4 colorRoughness;
};

GpuTri toGpuTri(ObjTriangle& tri, int matId) {
	return GpuTri{
		.pos0uv0x = glm::vec4(tri.pos0, tri.uv0.x),
		.pos1uv0y = glm::vec4(tri.pos1, tri.uv0.y),
		.pos2uv1x = glm::vec4(tri.pos2, tri.uv1.x),
		.normal0uv1y = glm::vec4(tri.normal0, tri.uv1.y),
		.normal1uv2x = glm::vec4(tri.normal1, tri.uv2.x),
		.normal2uv2y = glm::vec4(tri.normal2, tri.uv2.y),
		.materialId = glm::vec4(matId)
	};
}

export class GpuSceneSystem : public ExtEngineSystem {
public:
	GpuSceneSystem() {}

	virtual void start(ExtEngine& extEngine) override {
		m_triBuffer = new GpuBuffer<GpuTri>(1024);
		m_matBuffer = new GpuBuffer<GpuMat>(16);

		auto& cboxMeshes = extEngine.getSystem<AssetSystem>().findModel("cornellbox").meshes;

		std::vector<GpuTri> tris;

		for (int i = 0; i < cboxMeshes.size(); ++i) {
			for (auto& tri : cboxMeshes[i]) {
				tris.push_back(toGpuTri(tri, i < 6 ? 1 : 0));
			}
		}

		std::vector<GpuMat> mats = {
			{ .colorRoughness = glm::vec4(1, 1, 1, 0) },
			{ .colorRoughness = glm::vec4(1, 1, 1, 1) }
		};

		m_triCount = tris.size();

		m_triBuffer->setData(tris, 0, 0, m_triCount);
		m_matBuffer->setData(mats, 0, 0, mats.size());
	}

	virtual void stop() override {
		delete m_triBuffer;
		delete m_matBuffer;
	}

	GpuBuffer<GpuTri> *getTriBuffer() {
		return m_triBuffer;
	}

	GpuBuffer<GpuMat> *getMatBuffer() {
		return m_matBuffer;
	}

	int getTriCount() {
		return m_triCount;
	}
private:
	int m_triCount = 0;
	GpuBuffer<GpuTri> *m_triBuffer;
	GpuBuffer<GpuMat> *m_matBuffer;
};