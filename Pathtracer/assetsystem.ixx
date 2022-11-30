module;

#include <unordered_map>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <rapidobj.hpp>
#include <string>

export module assetsystem;

import extengine;
import shader;
import <typeinfo>;

std::string loadText(std::string path) {
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();

	return buffer.str();
}

export struct ObjTriangle {
public:
	glm::vec3 pos0;
	glm::vec3 pos1;
	glm::vec3 pos2;

	glm::vec3 normal0;
	glm::vec3 normal1;
	glm::vec3 normal2;

	glm::vec2 uv0;
	glm::vec2 uv1;
	glm::vec2 uv2;
};

export struct ObjModel {
public:
	std::vector<std::vector<ObjTriangle>> meshes;
};

export class ObjAssetLoader {
public:
	ObjAssetLoader(std::unordered_map<std::string, ObjModel>& modelMap) : m_modelMap(modelMap) {}

	void load(std::string name, std::string ext, std::string path) {
		auto result = rapidobj::ParseFile(path);

		std::string err = result.error.code.message();
		if (rapidobj::Triangulate(result)) {
			ObjModel model;
			ObjTriangle triangle;

			auto& poses = result.attributes.positions;
			auto& normals = result.attributes.normals;
			auto& uvs = result.attributes.texcoords;

			for (auto& shape : result.shapes) {
				std::vector<ObjTriangle> tris;
				for (int i = 0; i < shape.mesh.indices.size(); i += 3) {
					auto& index0 = shape.mesh.indices[i];
					auto& index1 = shape.mesh.indices[i + 1];
					auto& index2 = shape.mesh.indices[i + 2];

					triangle.pos0 = glm::vec3(
						poses[index0.position_index * 3], 
						poses[index0.position_index * 3 + 1], 
						poses[index0.position_index * 3 + 2]
					);

					triangle.pos1 = glm::vec3(
						poses[index1.position_index * 3],
						poses[index1.position_index * 3 + 1],
						poses[index1.position_index * 3 + 2]
					);

					triangle.pos2 = glm::vec3(
						poses[index2.position_index * 3],
						poses[index2.position_index * 3 + 1],
						poses[index2.position_index * 3 + 2]
					);

					triangle.normal0 = glm::vec3(
						normals[index0.normal_index * 3],
						normals[index0.normal_index * 3 + 1],
						normals[index0.normal_index * 3 + 2]
					);

					triangle.normal1 = glm::vec3(
						normals[index1.normal_index * 3],
						normals[index1.normal_index * 3 + 1],
						normals[index1.normal_index * 3 + 2]
					);

					triangle.normal2 = glm::vec3(
						normals[index2.normal_index * 3],
						normals[index2.normal_index * 3 + 1],
						normals[index2.normal_index * 3 + 2]
					);

					triangle.uv0 = glm::vec2(
						uvs[index0.texcoord_index * 2],
						uvs[index0.texcoord_index * 2 + 1]
					);

					triangle.uv1 = glm::vec2(
						uvs[index1.texcoord_index * 2],
						uvs[index1.texcoord_index * 2 + 1]
					);

					triangle.uv2 = glm::vec2(
						uvs[index2.texcoord_index * 2],
						uvs[index2.texcoord_index * 2 + 1]
					);

					tris.push_back(triangle);
				}

				model.meshes.push_back(tris);
			}

			m_modelMap[name] = model;
		}
	}

	std::vector<std::string> extensions() {
		return { ".obj" };
	}

	void postLoad() {
	}
private:
	std::unordered_map<std::string, ObjModel>& m_modelMap;
};

export class ProgramAssetLoader {
public:
	ProgramAssetLoader(std::unordered_map<std::string, GpuProgram*>& progMap) : m_progMap(progMap) {}

	void load(std::string name, std::string ext, std::string path) {
		std::string src = loadText(path);

		if (ext == ".vsh") {
			if (m_otherSource.contains(name)) {
				m_vertFragPairs.push_back({ name, src, m_otherSource[name] });
			} else {
				m_otherSource[name] = src;
			}
		} else if (ext == ".fsh") {
			if (m_otherSource.contains(name)) {
				m_vertFragPairs.push_back({ name, m_otherSource[name], src });
			} else {
				m_otherSource[name] = src;
			}
		} else if (ext == ".csh") {
			m_computes.push_back({ name, src });
		} else if (ext == ".glsl") {
			GpuProgram::addInclude(name, src);
		}
	}

	std::vector<std::string> extensions() {
		return { ".vsh", ".fsh", ".csh", ".glsl" };
	}

	void postLoad() {
		for (auto [name, vsh, fsh] : m_vertFragPairs) {
			m_progMap[name] = new GpuProgram(vsh, fsh);
		}

		for (auto [name, csh] : m_computes) {
			m_progMap[name] = new GpuProgram(csh);
		}

		m_vertFragPairs.clear();
		m_otherSource.clear();
	}
private:
	std::vector<std::pair<std::string, std::string>> m_computes;
	std::vector<std::tuple<std::string, std::string, std::string>> m_vertFragPairs;
	std::unordered_map<std::string, std::string> m_otherSource;
	std::unordered_map<std::string, GpuProgram *>& m_progMap;
};

export class AssetSystem : public ExtEngineSystem {
public:
	AssetSystem() : m_progLoader(m_programs), m_objLoader(m_models) {}

	void loadFrom(std::string folder) {
		auto progLoaderExts = m_progLoader.extensions();
		auto objLoaderExts = m_objLoader.extensions();

		auto root = std::filesystem::absolute(folder);
		for (std::filesystem::recursive_directory_iterator iter(root), end; iter != end; ++iter) {
			if (iter->is_regular_file()) {
				auto path = iter->path().string();
				auto ext = iter->path().extension().string();
				auto name = iter->path().lexically_relative(root).replace_extension("").string();

				std::replace(name.begin(), name.end(), static_cast<char>(iter->path().preferred_separator), '/');

				if (std::find(progLoaderExts.begin(), progLoaderExts.end(), ext) != progLoaderExts.end()) {
					m_progLoader.load(name, ext, path);
				}

				if (std::find(objLoaderExts.begin(), objLoaderExts.end(), ext) != objLoaderExts.end()) {
					m_objLoader.load(name, ext, path);
				}
			}
		}

		m_progLoader.postLoad();
		m_objLoader.postLoad();
	}

	GpuProgram *findProgram(std::string name) {
		return m_programs[name];
	}

	ObjModel& findModel(std::string name) {
		return m_models[name];
	}

	virtual void stop() override {
		for (auto& [_, prog] : m_programs) {
			delete prog;
		}
	}
private:
	std::unordered_map<std::string, GpuProgram *> m_programs;
	std::unordered_map<std::string, ObjModel> m_models;
	ProgramAssetLoader m_progLoader;
	ObjAssetLoader m_objLoader;
};