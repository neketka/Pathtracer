module;

#include <typeinfo>
#include <glm/glm.hpp>
#include <algorithm>
#include <vector>
#include <queue>
#include <random>
#include <bvh/sweep_sah_builder.hpp>
#include <bvh/triangle.hpp>
#include <glm/gtx/polar_coordinates.hpp>

export module gpuscenesystem;

import extengine;
import buffer;
import assetsystem;

export struct GpuTri
{
	glm::vec4 pos0normx;
	glm::vec4 pos1normy;
	glm::vec4 pos2normz;
	glm::uvec4 uvMatId;
};

export struct GpuBvhNode
{
	glm::vec4 minExtent;
	glm::vec4 maxExtent;
	glm::ivec4 navigation;
	// left, right, firstTri, triCount
};

export struct GpuMat
{
	glm::vec4 colorRM;
};

float toPolar(glm::vec3 normal)
{
	glm::vec3 p = glm::gtx::polar_coordinates::polar(normal);
	return glm::uintBitsToFloat(glm::packHalf2x16(glm::vec2(p.y, p.z)));
}

GpuTri toGpuTri(ObjTriangle &tri, int matId)
{
	glm::vec3 normal = (tri.normal0 + tri.normal1 + tri.normal2) / 3.f;
	auto n0 = toPolar(tri.normal0);
	auto n1 = toPolar(tri.normal1);
	auto n2 = toPolar(tri.normal2);
	auto uv0 = glm::packHalf2x16(tri.uv0);
	auto uv1 = glm::packHalf2x16(tri.uv1);
	auto uv2 = glm::packHalf2x16(tri.uv2);

	return GpuTri{
			.pos0normx = glm::vec4(tri.pos0, n0),
			.pos1normy = glm::vec4(tri.pos1, n1),
			.pos2normz = glm::vec4(tri.pos2, n2),
			.uvMatId = glm::uvec4(uv0, uv1, uv2, matId)};
}

bvh::Triangle<float> toBvhTri(GpuTri &tri)
{
	auto v1 = bvh::Vector3<float>(tri.pos0normx.x, tri.pos0normx.y, tri.pos0normx.z);
	auto v2 = bvh::Vector3<float>(tri.pos1normy.x, tri.pos1normy.y, tri.pos1normy.z);
	auto v3 = bvh::Vector3<float>(tri.pos2normz.x, tri.pos2normz.y, tri.pos2normz.z);

	return bvh::Triangle<float>(v1, v2, v3);
}

bvh::Bvh<float> buildBvh(std::vector<GpuTri> &tris)
{
	std::vector<bvh::Triangle<float>> prims;
	prims.reserve(tris.size());

	for (GpuTri &tri : tris)
	{
		prims.push_back(toBvhTri(tri));
	}
	// https://github.com/madmann91/bvh/blob/master/test/simple_example.cpp

	auto [bboxes, centers] = bvh::compute_bounding_boxes_and_centers(prims.data(), prims.size());
	auto global_bbox = bvh::compute_bounding_boxes_union(bboxes.get(), prims.size());

	bvh::Bvh<float> bvvh;

	bvh::SweepSahBuilder builder(bvvh);
	builder.build(global_bbox, bboxes.get(), centers.get(), prims.size());

	return bvvh;
}

void convertForGpu(bvh::Bvh<float> &bvvh, std::vector<GpuTri> &inTris, std::vector<GpuTri> &outTris, std::vector<GpuBvhNode> &outNodes)
{
	outTris.reserve(inTris.size());
	outNodes.resize(bvvh.node_count);
	for (int i = 0; i < bvvh.node_count; ++i)
	{
		auto &node = bvvh.nodes[i];
		auto &outNode = outNodes[i];
		outNode.minExtent = glm::vec4(node.bounds[0], node.bounds[2], node.bounds[4], 0.f);
		outNode.maxExtent = glm::vec4(node.bounds[1], node.bounds[3], node.bounds[5], 0.f);
		if (node.is_leaf())
		{
			outNode.navigation = glm::ivec4(-1, -1, outTris.size(), node.primitive_count);
			for (int i = 0; i < node.primitive_count; ++i)
			{
				int triIdx = bvvh.primitive_indices[node.first_child_or_primitive + i];
				outTris.push_back(inTris[triIdx]);
			}
		}
		else
		{
			int leftIdx = node.first_child_or_primitive;
			int rightIdx = bvh::Bvh<float>::sibling(leftIdx);
			outNode.navigation = glm::ivec4(leftIdx, rightIdx, 0, 0);
		}
	}
}

export class GpuSceneSystem : public ExtEngineSystem
{
public:
	GpuSceneSystem() {}

	virtual void start(ExtEngine &extEngine) override
	{
		m_triBuffer = new GpuBuffer<GpuTri>(1024);
		m_matBuffer = new GpuBuffer<GpuMat>(16);

		auto &cboxMeshes = extEngine.getSystem<AssetSystem>().findModel("cornellbox").meshes;
		auto &monkeyMeshes = extEngine.getSystem<AssetSystem>().findModel("suzanne").meshes;

		std::vector<GpuTri> tris;

		for (int i = 0; i < monkeyMeshes.size(); ++i)
		{
			for (auto &tri : monkeyMeshes[i])
			{
				tris.push_back(toGpuTri(tri, 0));
			}
		}

		for (int i = 0; i < cboxMeshes.size(); ++i)
		{
			for (auto &tri : cboxMeshes[i])
			{
				tris.push_back(toGpuTri(tri, 1));
			}
		}

		std::vector<GpuMat> mats = {
				{.colorRM = glm::vec4(0.8f, 0.8f, 0.8f, glm::uintBitsToFloat(glm::packHalf2x16(glm::vec2(0.0, 0.0))))},
				{.colorRM = glm::vec4(0.8f, 0.8f, 0.8f, glm::uintBitsToFloat(glm::packHalf2x16(glm::vec2(1.0, 1.0))))}};

		m_triCount = tris.size();

		auto bvvh = buildBvh(tris);

		std::vector<GpuBvhNode> bvhNodes;
		std::vector<GpuTri> triBuffer;

		convertForGpu(bvvh, tris, triBuffer, bvhNodes);

		m_bvhBuffer = new GpuBuffer<GpuBvhNode>(bvhNodes.size());
		m_bvhBuffer->setData(bvhNodes, 0, 0, bvhNodes.size());
		m_triBuffer->setData(triBuffer, 0, 0, m_triCount);
		m_matBuffer->setData(mats, 0, 0, mats.size());
	}

	virtual void stop() override
	{
		delete m_triBuffer;
		delete m_matBuffer;
	}

	GpuBuffer<GpuTri> *getTriBuffer()
	{
		return m_triBuffer;
	}

	GpuBuffer<GpuMat> *getMatBuffer()
	{
		return m_matBuffer;
	}

	GpuBuffer<GpuBvhNode> *getBvhBuffer()
	{
		return m_bvhBuffer;
	}

	int getTriCount()
	{
		return m_triCount;
	}

private:
	int m_triCount = 0;
	GpuBuffer<GpuTri> *m_triBuffer;
	GpuBuffer<GpuMat> *m_matBuffer;
	GpuBuffer<GpuBvhNode> *m_bvhBuffer;
};