module;

#include <typeinfo>
#include <glm/glm.hpp>

export module gpuscenesystem;

import extengine;
import buffer;
import assetsystem;

export struct GpuTri {
	glm::vec4 pos0normx;
	glm::vec4 pos1normy;
	glm::vec4 pos2normz;
	glm::uvec4 uvMatId;
};

export struct GpuMat {
	glm::vec4 colorRoughness;
};

GpuTri toGpuTri(ObjTriangle& tri, int matId) {
	glm::vec3 normal = (tri.normal0 + tri.normal1 + tri.normal2) / 3.f;
	auto uv0 = glm::packHalf2x16(tri.uv0);
	auto uv1 = glm::packHalf2x16(tri.uv1);
	auto uv2 = glm::packHalf2x16(tri.uv2);

	return GpuTri{
		.pos0normx = glm::vec4(tri.pos0, normal.x),
		.pos1normy = glm::vec4(tri.pos1, normal.y),
		.pos2normz = glm::vec4(tri.pos2, normal.z),
		.uvMatId = glm::uvec4(uv0, uv1, uv2, matId)
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
			{ .colorRoughness = glm::vec4(1.f, 1.f, 1.f, 0.f) },
			{ .colorRoughness = glm::vec4(0.8f, 0.8f, 0.8f, 1.f) }
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