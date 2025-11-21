#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <Windows.h>
#include <cstdio>
#include <vector>
#include <string>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#include <memory>
#include "TabControl.h"
#include "DocumentText.h"

class TextEditor {
public:
    CommandHistory commandHistory;
    HWND hMainWindow{};
    HMENU hMenu{};
    static HINSTANCE hInstance;
    TabControl* tabControl = nullptr;
    std::vector<std::unique_ptr<DocumentText>> documents;
    int m_nFontHeight = 16;
    static constexpr int LONGEST_LINE = 1024;

    TextEditor();
    void undo();
    void redo();
    void setCursorPosition(size_t position) const;
    void updateEditControl() const;
    void show() const;
    static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    static void setInstance(HINSTANCE hInst);
    LRESULT OnPaint(HWND hWnd) const;
    void SubclassEditControl(HWND hEditControl);
    DocumentText* currentDocument{};


private:
    static void registerWindowClass();
    void createMainWindow();
    void addMenus();
    void addControls();
    LRESULT handleMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    void handleException(const std::exception& e) const;
    void handleUnknownException() const;
    LRESULT handleCommand(WPARAM wp, LPARAM lp);
    void createNewTab();
    void openFile();
    void saveFile() const;
    void saveFileAs() const;
    void saveAllFiles() const;
    static void displayFile(const DocumentText* document, HWND editControl);
    void writeFile(const std::wstring& path, const DocumentText* document) const;
    void updateWindowTitle() const;
    LONG PaintLine(HDC hdc, ULONG nLineNo, const DocumentText* document, const RECT& clientRect) const;

    static LRESULT CALLBACK SubclassEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    [[nodiscard]] DocumentText* getCurrentDocument() const;

};

#endif // TEXTEDITOR_H
