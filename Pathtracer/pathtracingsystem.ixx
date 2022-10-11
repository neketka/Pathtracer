module;

#include <glm/glm.hpp>

export module pathtracingsystem;

import extengine;
import pathtracing;
import fullscreenquad;
import window;
import transforms;
import movementsystem;
import <typeinfo>;
import assetsystem;

export class PathtracingSystem : public ExtEngineSystem {
public:
	PathtracingSystem() {}

	virtual void start(ExtEngine& engine) override {
		auto assetSystem = engine.getSystem<AssetSystem>();

		m_fQuad = new FullScreenQuadPass(assetSystem.findProgram("fullquad"));
		m_pathtracing = new PathtracingPass(assetSystem.findProgram("pathmain"));
		m_pBuffer = new PathtracingBuffer;
		m_moveSystem = &engine.getSystem<MovementSystem>();
	}

	virtual void stop() override {
		delete m_fQuad;
		delete m_pathtracing;
		delete m_pBuffer;
	}

	virtual void resize(int w, int h) override {
		m_w = w;
		m_h = h;

		m_pBuffer->resize(w, h);
	}

	virtual void render(float deltaT, float fps) override {
		glm::mat4 view = m_moveSystem->getViewMatrix();
		m_pathtracing->render(view, m_pBuffer);
		m_fQuad->render(m_w, m_h, m_pBuffer->getColor());
	}

	void setConfig(PathtracingConfig config) {
		m_pathtracing->setConfig(config);
	}
private:
	int m_w = 800, m_h = 600;

	FullScreenQuadPass* m_fQuad;
	PathtracingPass* m_pathtracing;
	PathtracingBuffer* m_pBuffer;
	MovementSystem* m_moveSystem;
};