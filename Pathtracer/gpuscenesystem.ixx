module;

#include <typeinfo>
#include <glm/glm.hpp>
#include <algorithm>
#include <vector>

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

export struct GpuBvhNode {
	glm::vec4 minExtent;
	glm::vec4 maxExtent;
	glm::ivec4 navigation;
	// left, right, parent, tri
};

export struct GpuMat {
	glm::vec4 colorRoughness;
};

class BvhNode {
public:
	BvhNode(std::vector<GpuTri>& triangles, int start, int end) {
		minExtent = glm::vec3(triangles[start].pos0normx);
		maxExtent = minExtent;
		for (int i = start; i < end; ++i) {
			GpuTri& tri = triangles[i];

			minExtent = glm::min(minExtent, glm::vec3(tri.pos0normx));
			minExtent = glm::min(minExtent, glm::vec3(tri.pos1normy));
			minExtent = glm::min(minExtent, glm::vec3(tri.pos2normz));

			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos0normx));
			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos1normy));
			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos2normz));
		}

		if (start == end - 1) {
			triIndex = start;
		} else {
			int mid = (start + end) / 2;

			left = new BvhNode(triangles, start, mid);
			right = new BvhNode(triangles, mid, end);
		}
	}

	int calcIndices() {
		int index = 0;
		calcIndicesAux(index);
		return index;
	}

	void convertBvh(std::vector<GpuBvhNode>& nodes) {
		nodes[index].minExtent = glm::vec4(minExtent, 0.f);
		nodes[index].maxExtent = glm::vec4(maxExtent, 0.f);
		if (triIndex != -1) {
			nodes[index].navigation = glm::uvec4(-1, -1, parentIndex, triIndex);
		}
		else {
			left->parentIndex = index;
			right->parentIndex = index;

			nodes[index].navigation = glm::ivec4(left->index, right->index, parentIndex, -1);

			left->convertBvh(nodes);
			right->convertBvh(nodes);
		}
	}
private:
	glm::vec3 maxExtent;
	glm::vec3 minExtent;

	int index = -1;
	int parentIndex = 0;
	int triIndex = -1;

	BvhNode* left = nullptr;
	BvhNode* right = nullptr;

	void calcIndicesAux(int& indexCounter) {
		index = indexCounter++;
		if (left) {
			left->calcIndicesAux(indexCounter);
		}
		if (right) {
			right->calcIndicesAux(indexCounter);
		}
	}
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
		auto& monkeyMeshes = extEngine.getSystem<AssetSystem>().findModel("suzanne").meshes;

		std::vector<GpuTri> tris;

		for (int i = 0; i < cboxMeshes.size(); ++i) {
			for (auto& tri : cboxMeshes[i]) {
				tris.push_back(toGpuTri(tri, i < 6 ? 1 : 0));
			}
		}

		for (int i = 0; i < monkeyMeshes.size(); ++i) {
			for (auto& tri : monkeyMeshes[i]) {
				//tris.push_back(toGpuTri(tri, 1));
			}
		}

		std::vector<GpuMat> mats = {
			{ .colorRoughness = glm::vec4(1.f, 1.f, 1.f, 0.f) },
			{ .colorRoughness = glm::vec4(0.8f, 0.8f, 0.8f, 1.f) }
		};

		m_triCount = tris.size();

		std::sort(tris.begin(), tris.end(), [](GpuTri& a, GpuTri& b) {
			auto center1 = glm::vec3(a.pos0normx + a.pos1normy + a.pos2normz) / 3.f;
			auto center2 = glm::vec3(b.pos0normx + b.pos1normy + b.pos2normz) / 3.f;

			auto d1 = glm::dot(glm::vec3(1, 0, 0), center1);
			auto d2 = glm::dot(glm::vec3(1, 0, 0), center2);

			return d1 < d2;
		});

		BvhNode bvh(tris, 0, m_triCount);
		std::vector<GpuBvhNode> bvhNodes(bvh.calcIndices());
		bvh.convertBvh(bvhNodes);
		
		m_bvhBuffer = new GpuBuffer<GpuBvhNode>(bvhNodes.size());
		m_bvhBuffer->setData(bvhNodes, 0, 0, bvhNodes.size());
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

	GpuBuffer<GpuBvhNode> *getBvhBuffer() {
		return m_bvhBuffer;
	}

	int getTriCount() {
		return m_triCount;
	}
private:
	int m_triCount = 0;
	GpuBuffer<GpuTri> *m_triBuffer;
	GpuBuffer<GpuMat> *m_matBuffer;
	GpuBuffer<GpuBvhNode> *m_bvhBuffer;
};