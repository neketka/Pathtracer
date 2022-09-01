module;

#include <SDL2/SDL.h>
#include <gl/glew.h>

#include <string>
#include <chrono>

export module window;

export template <class TEngine>
class Window {
public:
	Window(std::string title, int w, int h, TEngine &engine) : m_engInst(engine) {
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

		m_window = SDL_CreateWindow(title.c_str(), 100, 100, w, h, SDL_WINDOW_OPENGL);
		m_context = SDL_GL_CreateContext(m_window);
		m_running = true;

		SDL_GL_SetSwapInterval(1);
	}

	~Window() {
		SDL_DestroyWindow(m_window);
	}

	void start() {
		m_engInst.start();

		SDL_Event e;

		auto lastFrame = std::chrono::high_resolution_clock::now();

		float fps = 0;

		int samples = 0;
		float sampledFrametime = 0;

		while (m_running) {
			while (SDL_PollEvent(&e)) {
				switch (e.type)
				{
				case SDL_QUIT:
					m_running = false;
					break;
				}
			}

			if (!m_running) {
				m_engInst.stop();
				break;
			}

			auto curFrame = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> timeSince = curFrame - lastFrame;
			float secSince = timeSince.count();

			if (sampledFrametime >= 1.0) {
				fps = samples / sampledFrametime;
				samples = 0;
				sampledFrametime = 0;
			}

			sampledFrametime += secSince;
			++samples;
			lastFrame = curFrame;

			m_engInst.tick(secSince, fps);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			m_engInst.render(secSince, fps);

			SDL_GL_SwapWindow(m_window);
		}
	}

private:
	TEngine &m_engInst;
	SDL_Window *m_window;
	SDL_GLContext m_context;
	bool m_running;
};