#ifndef TABCONTROL_H
#define TABCONTROL_H

#include <Windows.h>
#include <vector>
#include <string>
#include <commctrl.h>
#include <functional>

#pragma comment(lib, "Comctl32.lib")

class TabControl {
public:
    TabControl(HWND parentWindow);

    void addTab(const std::wstring& title, HWND content, const std::wstring& filePath = L"");
    void removeTab(int index);
    int getCurrentTabIndex() const;
    void setCurrentTab(int index);
    int getTabCount();
    HWND getTabControlHandle() const;
    HWND getCurrentEditControl() const;
    void setCurrentFilePath(const std::wstring& path);
    std::vector<std::wstring>& getFilePaths();
    std::wstring getCurrentFilePath();
    void changeTabName(const std::wstring& filePath);
    void Resize(int width, int height);
    void DrawTabWithCloseButton(HDC hdc, const RECT& rect, LPCTSTR text, bool isSelected);
    std::function<void(int)> onTabChanged;
    std::function<void(int)> onTabRemoved;

private:
    HWND hTabControl;
    WNDPROC OldTabProc;
    std::vector<HWND> tabContents;
    std::vector<std::wstring> filePaths;

    static LRESULT CALLBACK TabProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void showTabContent(int index);
    LRESULT OnPaint(HWND hWnd);
};

#endif // TABCONTROL_H
