module;

#include <glm/glm.hpp>

#include <vector>
#include <string>

export module demosystem;

import extengine;
import pathtracing;
import pathtracingsystem;
import window;
import <typeinfo>;

export class DemoSystem : public ExtEngineSystem {
public:
	DemoSystem() {}

	virtual void start(ExtEngine& engine) override {
		m_window = &engine.getWindow();
		m_pathSystem = &engine.getSystem<PathtracingSystem>();
		setConfig();
	}

	virtual void key(int id, bool keyDown) override {
		if (id == 46 && !keyDown) {
			m_configStage++;
			setConfig();
		}
		else if (id == 45 && !keyDown) {
			m_configStage--;
			setConfig();
		}
	}

	void setConfig() {
		PathtracingConfig config;

		m_configStage = glm::clamp<int>(m_configStage, 0, 8);
		switch (m_configStage) {
		case 0:
			config.bounces = 0;
			config.jitter = 0;
			config.shadowSamples = 1;
			config.lightRadius = 0.f;
			m_window->setTitle("Pathtracer: Bounces=0; JitterAA=0; ShadowSamples=1; LightRadius=0.0;");
			break;
		case 1:
			config.bounces = 0;
			config.jitter = 0;
			config.shadowSamples = 1;
			m_window->setTitle("Pathtracer: Bounces=0; JitterAA=0; ShadowSamples=1; LightRadius=1.0;");
			break;
		case 2:
			config.bounces = 0;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=0; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 3:
			config.bounces = 1;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=1; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 4:
			config.bounces = 2;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=2; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 5:
			config.bounces = 3;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=3; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 6:
			config.bounces = 4;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=4; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 7:
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=5; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 8:
			m_window->setTitle("Pathtracer: Bounces=5; JitterAA=0.5; ShadowSamples=2; LightRadius=1.0;");
			break;
		}

		m_pathSystem->setConfig(config);
	}
private:
	PathtracingSystem* m_pathSystem;
	Window<ExtEngine>* m_window;
	int m_configStage = 0;
};