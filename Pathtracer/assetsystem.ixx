module;

#include <unordered_map>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

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
	AssetSystem() : m_progLoader(m_programs) {}

	void loadFrom(std::string folder) {
		auto progLoaderExts = m_progLoader.extensions();

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
			}
		}

		m_progLoader.postLoad();
	}

	GpuProgram *findProgram(std::string name) {
		return m_programs[name];
	}

	virtual void stop() override {
		for (auto& [_, prog] : m_programs) {
			delete prog;
		}
	}
private:
	std::unordered_map<std::string, GpuProgram *> m_programs;
	ProgramAssetLoader m_progLoader;
};