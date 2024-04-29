#include "Window.h"

Window::Window(HINSTANCE hInstance, int nCmdShow) : hInstance_(hInstance), nCmdShow_(nCmdShow) {}
Canvas::Canvas(HDC hdc) : hdc_(hdc) {}
UpdateCallback Window::updateCallback_;
InputState Window::inputState;
int Window::fps_;

bool Window::CreateWindowAndRun() {
	if (!RegisterWindowClass()) {
		return false;
	}

	if (!CreateMainWindow()) {
		return false;
	}

	ShowWindow(hWnd_, nCmdShow_);
	UpdateWindow(hWnd_);

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

void Window::RegisterUpdateFunction(int fps, UpdateCallback update) {
	fps_ = fps;
	updateCallback_ = update;
}

Vector2i Window::getSize()
{
	RECT windowRect;
	GetWindowRect(hWnd_, &windowRect);
	return { windowRect.left + windowRect.right ,windowRect.top + windowRect.bottom };
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_MOUSEWHEEL: {
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		inputState.scrollDirection = clamp(zDelta / 120, -1, 1);
		break;
	}
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		// Create a compatible device context for the back buffer
		HDC backBufferDC = CreateCompatibleDC(hdc);

		// Create a compatible bitmap for the back buffer
		HBITMAP backBufferBitmap = CreateCompatibleBitmap(hdc, clientWidth, clientHeight);

		// Select the back buffer bitmap into the back buffer device context
		HBITMAP oldBackBufferBitmap = (HBITMAP)SelectObject(backBufferDC, backBufferBitmap);

		HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(255, 255, 255)); // Example: White background
		RECT backBufferRect = { 0, 0, clientWidth, clientHeight };
		FillRect(backBufferDC, &backBufferRect, hBackgroundBrush);
		DeleteObject(hBackgroundBrush);
		// Perform drawing operations on the back buffer
		if (updateCallback_)
			updateCallback_(backBufferDC);
		inputState.scrollDirection = 0;
		// Copy the content of the back buffer to the window's device context
		BitBlt(hdc, 0, 0, clientWidth, clientHeight, backBufferDC, 0, 0, SRCCOPY);

		// Clean up
		SelectObject(backBufferDC, oldBackBufferBitmap);
		DeleteObject(backBufferBitmap);
		DeleteDC(backBufferDC);

		EndPaint(hWnd, &ps);

		Sleep(1000 / fps_);
		InvalidateRect(hWnd, nullptr, false);
	}break;
	case WM_KEYDOWN:
		inputState.keys[wParam] = true;
		break;
	case WM_KEYUP:
		inputState.keys[wParam] = false;
		break;
	case WM_MOUSEMOVE:
		inputState.mouseX = LOWORD(lParam);
		inputState.mouseY = HIWORD(lParam);
		break;
	case WM_LBUTTONDOWN:
		inputState.leftButtonDown = true;
		break;
	case WM_LBUTTONUP:
		inputState.leftButtonDown = false;
		break;
	case WM_RBUTTONDOWN:
		inputState.rightButtonDown = true;
		break;
	case WM_RBUTTONUP:
		inputState.rightButtonDown = false;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool Window::RegisterWindowClass() {
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance_;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"Win32WindowClass";

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	return true;
}

bool Window::CreateMainWindow() {
	hWnd_ = CreateWindowEx(
		0,
		L"Win32WindowClass",
		L"Win32 Window",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance_,
		NULL);

	if (!hWnd_) {
		MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONERROR | MB_OK);
		return false;
	}

	return true;
}

void Canvas::DrawLine(const Vector2f& pos1, const Vector2f& pos2) {
	MoveToEx(hdc_, pos1.x, pos1.y, nullptr);
	LineTo(hdc_, pos2.x, pos2.y);
}