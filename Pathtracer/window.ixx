module;

#include <SDL2/SDL.h>
#include <gl/glew.h>
#include <Windows.h>

#include <string>
#include <chrono>

export module window;

void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_HIGH)
		MessageBoxA(nullptr, message, "OpenGL Error", MB_OK);
}

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

		m_window = SDL_CreateWindow(title.c_str(), 100, 100, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		m_context = SDL_GL_CreateContext(m_window);
		m_running = true;

		SDL_GL_SetSwapInterval(1);
	}

	~Window() {
		SDL_DestroyWindow(m_window);
	}

	void start() {
		glewInit();

		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(MessageCallback, 0);

		int w, h;
		SDL_GetWindowSize(m_window, &w, &h);

		m_engInst.start(*this);
		m_engInst.resize(w, h);

		SDL_Event e;

		auto lastFrame = std::chrono::high_resolution_clock::now();

		float fps = 0;

		int samples = 0;
		float sampledFrametime = 0;

		while (m_running) {
			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_QUIT) {
					m_running = false;
				} else if (e.type == SDL_WINDOWEVENT) {
					switch (e.window.event) { 
					case SDL_WINDOWEVENT_RESIZED:
						w = e.window.data1, h = e.window.data2;
						m_engInst.resize(w, h);
						break;
					}		
				} else if (e.type == SDL_MOUSEBUTTONDOWN) {
					m_engInst.mouseButton(e.button.button == SDL_BUTTON_LEFT, e.button.button == SDL_BUTTON_RIGHT, true);
				} else if (e.type == SDL_MOUSEBUTTONUP) {
					m_engInst.mouseButton(e.button.button == SDL_BUTTON_LEFT, e.button.button == SDL_BUTTON_RIGHT, false);
				} else if (e.type == SDL_MOUSEMOTION) {
					m_engInst.mouseMove(e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel);
				} else if (e.type == SDL_KEYDOWN) {
					m_engInst.key(e.key.keysym.scancode, true);
				} else if (e.type == SDL_KEYUP) {
					m_engInst.key(e.key.keysym.scancode, false);
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

	void stop() {
		m_running = false;
	}

	void captureMouse(bool state) {
		SDL_SetRelativeMouseMode(state ? SDL_TRUE : SDL_FALSE);
	}

private:
	TEngine &m_engInst;
	SDL_Window *m_window;
	SDL_GLContext m_context;
	bool m_running;
};