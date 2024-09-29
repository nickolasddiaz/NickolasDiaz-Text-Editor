#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <commctrl.h>
#include <winuser.h>
#pragma comment(lib, "Comctl32.lib")
#include <memory>
#include <functional>
#include <commctrl.h>
#include <windowsx.h>
#include <iostream>
using namespace std;
#include "TextEditor.h"

HINSTANCE TextEditor::hInstance = NULL;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    TextEditor::setInstance(hInstance);
    TextEditor editor;
    editor.show();

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}