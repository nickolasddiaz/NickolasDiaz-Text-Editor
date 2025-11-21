#include <Windows.h>
#include <string>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#include <memory>
#include "TextEditor.h"

HINSTANCE TextEditor::hInstance = nullptr;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    TextEditor::setInstance(hInstance);
    TextEditor editor;
    editor.show();

    MSG msg = { nullptr };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}