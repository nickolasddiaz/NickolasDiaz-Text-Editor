#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

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
#include <iostream>
using namespace std;
#include "TabControl.h"
#include "DocumentText.h"

class TextEditor {
public:
    HWND hMainWindow;
    HMENU hMenu;
    static HINSTANCE hInstance;
    TabControl* tabControl = nullptr;
    vector<unique_ptr<DocumentText>> documents;
    int m_nFontHeight = 16;
    static const int LONGEST_LINE = 1024;

    TextEditor();
    void show();
    static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    static void setInstance(HINSTANCE hInst);
    LRESULT OnPaint(HWND hWnd);
    void SubclassEditControl(HWND hEditControl);
    DocumentText* currentDocument;


private:
    void registerWindowClass();
    void createMainWindow();
    void addMenus();
    void addControls();
    LRESULT handleMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    void handleException(const std::exception& e);
    void handleUnknownException();
    LRESULT handleCommand(WPARAM wp, LPARAM lp);
    void createNewTab();
    void openFile();
    void saveFile();
    void saveFileAs();
    void saveAllFiles();
    void displayFile(DocumentText* document, HWND editControl);
    void writeFile(const std::wstring& path, DocumentText* document);
    void updateWindowTitle();
    LONG PaintLine(HDC hdc, ULONG nLineNo, DocumentText* document, const RECT& clientRect);

    static LRESULT CALLBACK SubclassEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    void SynchronizeDocumentWithEdit(HWND hEditControl);


};

#endif // TEXTEDITOR_H
