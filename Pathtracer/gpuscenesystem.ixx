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

struct bestAxis {
	float cost;
	std::vector<GpuTri> leftTris;
	std::vector<GpuTri> rightTris;
	std::vector<int> leftIndexList;
	std::vector<int> rightIndexList;
};

class BvhNode {
public:
	BvhNode(std::vector<GpuTri>& triangles, std::vector<int> indexList) {
		minExtent = glm::vec3(triangles[indexList[0]].pos0normx);
		maxExtent = minExtent;

		for (int i = 0; i < triangles.size(); ++i) {
			GpuTri& tri = triangles[i];

			minExtent = glm::min(minExtent, glm::vec3(tri.pos0normx));
			minExtent = glm::min(minExtent, glm::vec3(tri.pos1normy));
			minExtent = glm::min(minExtent, glm::vec3(tri.pos2normz));

			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos0normx));
			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos1normy));
			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos2normz));
		}

		triIndexList = indexList;

		if ( triangles.size() == 1) {
			//triIndex = start;
			triIndex = triIndexList[0];
		}
		else {
			/*int mid = (start + end) / 2;

			left = new BvhNode(triangles, start, mid);
			right = new BvhNode(triangles, mid, end);*/
			bestAxis b = bestSplit(triangles, indexList);
			
			left = new BvhNode(b.leftTris, b.leftIndexList);
			right = new BvhNode(b.rightTris, b.rightIndexList);
		
		}
	}

	int calcIndices() {
		int index = 0;
		calcIndicesAux(index);
		return index;
	}

	bestAxis bestSplit(std::vector<GpuTri>& triangles, std::vector<int> indexList) {
		bestAxis best;
		float bestCost = std::numeric_limits<float>::max();
		for (int a = 0; a < 3; ++a) {
			for (int i = 0; i < triangles.size(); ++i) {
				GpuTri& tri = triangles[i];
				glm::vec3 candidatePos(tri.pos0normx + tri.pos1normy + tri.pos2normz);
				bestAxis b = evaluateSAH(triangles, a, candidatePos[a]/3, indexList);
				if (b.cost < bestCost) {
					best = b;
				}
			}
		}
		return best;
	}

	bestAxis evaluateSAH(std::vector<GpuTri>& triangles, int axis, int candidatePos, std::vector<int> indexList) {
		int leftCount = 0;
		int rightCount = 0;
		std::vector<GpuTri> rightTris;
		std::vector<GpuTri> leftTris;
		std::vector<int> rightIndexList;
		std::vector<int> leftIndexList;
		
		for (int i = 0; i < triangles.size(); ++i) {
			GpuTri& tri = triangles[i];
			glm::vec3 baryCenter(tri.pos0normx + tri.pos1normy + tri.pos2normz);
			int pos = baryCenter[axis] / 3;
			if (pos < candidatePos) {
				leftCount++;
				leftTris.push_back(tri);
				leftIndexList.push_back(indexList[i]);
			}
			else {
				rightCount++;
				rightTris.push_back(tri);
				rightIndexList.push_back(indexList[i]);
			}
		}
		float cost = leftCount * calculateArea(leftTris) + rightCount * calculateArea(rightTris);
		return {
			.cost = cost,
			.leftTris = leftTris,
			.rightTris = rightTris,
			.leftIndexList = leftIndexList,
			.rightIndexList = rightIndexList,
		};
	}

	float calculateArea(std::vector<GpuTri> triangles) {
		glm::vec3 maxExtent;
		glm::vec3 minExtent;
		for (int i = 0; i < triangles.size(); ++i) {
			GpuTri& tri = triangles[i];

			minExtent = glm::min(minExtent, glm::vec3(tri.pos0normx));
			minExtent = glm::min(minExtent, glm::vec3(tri.pos1normy));
			minExtent = glm::min(minExtent, glm::vec3(tri.pos2normz));

			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos0normx));
			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos1normy));
			maxExtent = glm::max(maxExtent, glm::vec3(tri.pos2normz));
		}
		glm::vec3 diff = maxExtent - minExtent;
		return diff[0] * diff[1] + diff[1] * diff[2] + diff[0] * diff[2];
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
	std::vector<int> triIndexList;

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

		std::vector<int> indexes(m_triCount);
		for (int i = 0; i < m_triCount; i++)
		{
			indexes[i] = i;
		}

		BvhNode bvh(tris, indexes);
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