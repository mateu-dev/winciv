#ifndef WIN32_WINDOW_H
#define WIN32_WINDOW_H

#include "framework.h"

typedef std::function<void(HDC&)> UpdateCallback;

struct InputState {
	bool keys[256];
	int mouseX;
	int mouseY;
	int scrollDirection;
	bool leftButtonDown;
	bool rightButtonDown;
	bool isScrolling;
};

class Window {
public:
	Window(HINSTANCE, int);
	bool CreateWindowAndRun();
	static void RegisterUpdateFunction(int,UpdateCallback);
	Vector2i getSize();
	static InputState inputState;
private:
	static UpdateCallback updateCallback_;
	static int fps_;
	static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
	bool RegisterWindowClass();
	bool CreateMainWindow();

	HINSTANCE hInstance_;
	HWND hWnd_;
	int nCmdShow_;
};



class Canvas {
public:
	Canvas(HDC hdc);
	void DrawLine(const Vector2f& pos1, const Vector2f& pos2);
	HDC hdc_;
private:
};

template <typename T>
T clamp(const T& n, const T& lower, const T& upper) {
	return max(lower, min(n, upper));
}

#endif // WIN32_WINDOW_H