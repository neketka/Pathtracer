module;

#include <vector>
#include <unordered_map>

export module extengine;

import window;
import <typeinfo>;

class ExtEngine;

export class ExtEngineSystem {
public:
	virtual ~ExtEngineSystem() {
	}
protected:
	virtual void start(ExtEngine& engine) {
	}

	virtual void stop() {
	}

	virtual void resize(int w, int h) {
	}

	virtual void key(int id, bool keyDown) {
	}

	virtual void mouseButton(bool leftButton, bool rightButton, bool buttonDown) {
	}

	virtual void mouseMove(int x, int y, int dX, int dY) {
	}

	virtual void tick(float deltaT, float fps) {
	}

	virtual void render(float deltaT, float fps) {
	}

	friend class ExtEngine;
};

export class ExtEngine {
public:
	~ExtEngine() {
		for (auto& [_, sys] : m_systems) {
			delete sys;
		}
	}

	Window<ExtEngine>& getWindow() {
		return *m_window;
	}

	template<class T>
	T& getSystem() {
		return *static_cast<T *>(sysById(typeid(T).hash_code()));
	}
protected:
	ExtEngineSystem *sysById(std::size_t id) {
		return m_systems[id];
	}

	ExtEngine() {}
	std::unordered_map<std::size_t, ExtEngineSystem *> m_systems;
private:
	friend class Window<ExtEngine>;

	void start(Window<ExtEngine>& window) {
		m_window = &window;
		for (auto& [_, sys] : m_systems) {
			sys->start(*this);
		}
	}

	void stop() {
		for (auto& [_, sys] : m_systems) {
			sys->stop();
		}
	}

	void resize(int w, int h) {
		for (auto& [_, sys] : m_systems) {
			sys->resize(w, h);
		}
	}

	void key(int id, bool keyDown) {
		for (auto& [_, sys] : m_systems) {
			sys->key(id, keyDown);
		}
	}

	void mouseButton(bool leftButton, bool rightButton, bool buttonDown) {
		for (auto& [_, sys] : m_systems) {
			sys->mouseButton(leftButton, rightButton, buttonDown);
		}
	}

	void mouseMove(int x, int y, int dX, int dY) {
		for (auto& [_, sys] : m_systems) {
			sys->mouseMove(x, y, dX, dY);
		}
	}

	void tick(float deltaT, float fps) {
		for (auto& [_, sys] : m_systems) {
			sys->tick(deltaT, fps);
		}
	}

	void render(float deltaT, float fps) {
		for (auto& [_, sys] : m_systems) {
			sys->render(deltaT, fps);
		}
	}

	Window<ExtEngine> *m_window;
};

export template<class ...T> class ExtEngineBase : public ExtEngine {
public:
	ExtEngineBase() {
		addSystems<T...>();
	}
private:
	template<class ...TSystem>
	void addSystems() {
		(..., (m_systems[typeid(TSystem).hash_code()] = static_cast<ExtEngineSystem*>(new TSystem)));
	}
};