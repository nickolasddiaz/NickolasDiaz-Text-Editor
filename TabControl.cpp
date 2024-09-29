#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#include <memory>
#include <functional>

#include "TabControl.h"


    TabControl::TabControl(HWND parentWindow) {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_TAB_CLASSES;
        InitCommonControlsEx(&icex);

        hTabControl = CreateWindow(WC_TABCONTROL, L"",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
            0, 0, 0, 0, parentWindow, NULL, GetModuleHandle(NULL), NULL);

        OldTabProc = (WNDPROC)SetWindowLongPtr(hTabControl, GWLP_WNDPROC, (LONG_PTR)TabProc);
        SetWindowLongPtr(hTabControl, GWLP_USERDATA, (LONG_PTR)this);

        RECT rcClient;
        GetClientRect(parentWindow, &rcClient);
        SetWindowPos(hTabControl, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);

        SendMessage(hTabControl, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_FLATSEPARATORS);
        SendMessage(hTabControl, TCM_SETPADDING, 0, MAKELPARAM(20, 7));

    }

    void TabControl::addTab(const std::wstring& title, HWND content, const std::wstring& filePath) {
        TCITEM tie;
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<wchar_t*>(title.c_str());
        TabCtrl_InsertItem(hTabControl, TabCtrl_GetItemCount(hTabControl), &tie);
        tabContents.push_back(content);
        filePaths.push_back(filePath);

        // Get the display area of the tab control
        RECT rcDisplay;
        GetClientRect(hTabControl, &rcDisplay);
        TabCtrl_AdjustRect(hTabControl, FALSE, &rcDisplay);

        // Set the position and size of the new content window
        SetWindowPos(content, NULL,
            rcDisplay.left, rcDisplay.top,
            rcDisplay.right - rcDisplay.left,
            rcDisplay.bottom - rcDisplay.top,
            SWP_NOZORDER);

        if (tabContents.size() == 1) {
            ShowWindow(content, SW_SHOW);
        }
        else {
            ShowWindow(content, SW_HIDE);
        }

        InvalidateRect(hTabControl, NULL, TRUE);
    }
    void TabControl::removeTab(int index) {
        if (index >= 0 && index < tabContents.size()) {
            // Remove the tab and associated window
            TabCtrl_DeleteItem(hTabControl, index);
            DestroyWindow(tabContents[index]);
            tabContents.erase(tabContents.begin() + index);
            filePaths.erase(filePaths.begin() + index);

            // Notify the TextEditor class to handle document removal
            if (onTabRemoved) {
                onTabRemoved(index);
            }

            // Ensure a valid tab is selected if any remain
            int tabCount = tabContents.size();
            if (tabCount > 0) {
                if (index >= tabCount) {
                    setCurrentTab(tabCount - 1);  // Select the last tab
                }
                else {
                    setCurrentTab(index);  // Select the next valid tab
                }
            }
            else {
                setCurrentTab(-1);  // No tabs left
            }
        }
    }

    int TabControl::getCurrentTabIndex() const {
        return TabCtrl_GetCurSel(hTabControl);
    }

    void TabControl::setCurrentTab(int index) {
        if (index >= 0 && index < tabContents.size()) {
            TabCtrl_SetCurSel(hTabControl, index);
            showTabContent(index);
        }
        if (onTabChanged) {
            onTabChanged(index);
        }
    }

    int TabControl::getTabCount() {
        return tabContents.size();
    }

    HWND TabControl::getTabControlHandle() const {
        return hTabControl != nullptr ? hTabControl : NULL;
    }

    HWND TabControl::getCurrentEditControl() const {
        int currentIndex = TabCtrl_GetCurSel(hTabControl);
        if (currentIndex >= 0 && currentIndex < tabContents.size()) {
            return tabContents[currentIndex];
        }
        return NULL;
    }

    void TabControl::setCurrentFilePath(const std::wstring& path) {
        int currentIndex = getCurrentTabIndex();
        if (currentIndex >= 0 && currentIndex < filePaths.size()) {
            filePaths[currentIndex] = path;
        }
    }

    std::vector<std::wstring>& TabControl::getFilePaths() {
        return filePaths;
    }

    std::wstring TabControl::getCurrentFilePath() {
        int index = getCurrentTabIndex();
        if (index >= 0 && index < filePaths.size()) {
            return filePaths[index];
        }
        return L"";
    }

    void TabControl::changeTabName(const std::wstring& filePath) {
        TCITEM tie;
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<LPWSTR>(filePath.c_str());
        TabCtrl_SetItem(hTabControl, getCurrentTabIndex(), &tie);
        InvalidateRect(hTabControl, NULL, TRUE);
    }
    void TabControl::Resize(int width, int height) {
        if (hTabControl) {
            // Resize the tab control itself
            SetWindowPos(hTabControl, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);

            // Get the rectangle for the tab control
            RECT rcTabControl;
            GetClientRect(hTabControl, &rcTabControl);

            // Adjust the rectangle to exclude the tab headers
            RECT rcDisplay = rcTabControl;
            TabCtrl_AdjustRect(hTabControl, FALSE, &rcDisplay);

            // Resize and reposition all tab contents
            for (HWND hContent : tabContents) {
                SetWindowPos(hContent, NULL,
                    rcDisplay.left, rcDisplay.top,
                    rcDisplay.right - rcDisplay.left,
                    rcDisplay.bottom - rcDisplay.top,
                    SWP_NOZORDER);
            }
        }
    }
   


    void TabControl::DrawTabWithCloseButton(HDC hdc, const RECT& rect, LPCTSTR text, bool isSelected) {
        HBRUSH brush = CreateSolidBrush(isSelected ? RGB(255, 255, 255) : RGB(240, 240, 240));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, rect.left, rect.bottom, NULL);
        LineTo(hdc, rect.left, rect.top);
        LineTo(hdc, rect.right, rect.top);
        LineTo(hdc, rect.right, rect.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        RECT textRect = rect;
        textRect.right -= 20;
        DrawText(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT closeRect = rect;
        closeRect.left = rect.right - 16;
        closeRect.right = rect.right - 4;
        closeRect.top += 4;
        closeRect.bottom -= 4;
        DrawText(hdc, TEXT("×"), -1, &closeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    HWND hTabControl;
    WNDPROC OldTabProc;
    std::vector<HWND> tabContents;
    std::vector<std::wstring> filePaths;

    LRESULT CALLBACK TabControl::TabProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        TabControl* pThis = (TabControl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

        switch (message) {
        case WM_NOTIFY: {
            NMHDR* nmhdr = (NMHDR*)lParam;
            if (nmhdr->code == TCN_SELCHANGE) {
                int index = TabCtrl_GetCurSel(hWnd);
                pThis->showTabContent(index);
                return 0;
            }
            break;
        }
        case WM_PAINT:
            return pThis->OnPaint(hWnd);

        case WM_MOUSEMOVE: {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            TCHITTESTINFO hitTestInfo = { 0 };
            hitTestInfo.pt = pt;
            int tabIndex = TabCtrl_HitTest(hWnd, &hitTestInfo);

            static int lastHoveredTab = -1;

            if (tabIndex != lastHoveredTab) {
                if (lastHoveredTab != -1) {
                    RECT lastTabRect;
                    TabCtrl_GetItemRect(hWnd, lastHoveredTab, &lastTabRect);
                    InvalidateRect(hWnd, &lastTabRect, FALSE);
                }
                if (tabIndex != -1) {
                    RECT currentTabRect;
                    TabCtrl_GetItemRect(hWnd, tabIndex, &currentTabRect);
                    InvalidateRect(hWnd, &currentTabRect, FALSE);
                }
                lastHoveredTab = tabIndex;
            }

            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            TCHITTESTINFO hitTestInfo;
            hitTestInfo.pt = pt;
            int tabIndex = TabCtrl_HitTest(hWnd, &hitTestInfo);
            if (tabIndex >= 0) {
                RECT rect;
                TabCtrl_GetItemRect(hWnd, tabIndex, &rect);

                RECT closeRect = rect;
                closeRect.left = rect.right - 16;
                closeRect.right = rect.right - 4;
                closeRect.top += 4;
                closeRect.bottom -= 4;

                if (PtInRect(&closeRect, pt)) {
                    pThis->removeTab(tabIndex);
                    InvalidateRect(hWnd, NULL, TRUE);
                    return 0;
                }
            }

            break;
        }
        }

        return CallWindowProc(pThis->OldTabProc, hWnd, message, wParam, lParam);
    }

    void TabControl::showTabContent(int index) {
        for (int i = 0; i < tabContents.size(); ++i) {
            ShowWindow(tabContents[i], i == index ? SW_SHOW : SW_HIDE);
        }
        RECT rcDisplay;
        GetClientRect(hTabControl, &rcDisplay);
        TabCtrl_AdjustRect(hTabControl, FALSE, &rcDisplay);
        SetWindowPos(tabContents[index], NULL,
            rcDisplay.left, rcDisplay.top,
            rcDisplay.right - rcDisplay.left,
            rcDisplay.bottom - rcDisplay.top,
            SWP_NOZORDER);
    }

    LRESULT TabControl::OnPaint(HWND hWnd) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Paint the tab control
        RECT tabRect;
        GetClientRect(getTabControlHandle(), &tabRect);

        // Create a memory DC for double buffering
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, tabRect.right, tabRect.bottom);
        HGDIOBJ hOldBitmap = SelectObject(hdcMem, hbmMem);

        // Paint the tab control background
        FillRect(hdcMem, &tabRect, (HBRUSH)(COLOR_WINDOW + 1));

        // Draw the tabs
        int tabCount = TabCtrl_GetItemCount(getTabControlHandle());
        int selectedTab = getCurrentTabIndex();

        for (int i = 0; i < tabCount; i++) {
            RECT itemRect;
            TabCtrl_GetItemRect(getTabControlHandle(), i, &itemRect);

            TCHAR tabText[256];
            TCITEM tie = { 0 };
            tie.mask = TCIF_TEXT;
            tie.pszText = tabText;
            tie.cchTextMax = sizeof(tabText) / sizeof(TCHAR);
            TabCtrl_GetItem(getTabControlHandle(), i, &tie);

            bool isSelected = (i == selectedTab);
            DrawTabWithCloseButton(hdcMem, itemRect, tabText, isSelected);
        }

        // Draw the content area border
        RECT contentRect = tabRect;
        TabCtrl_AdjustRect(getTabControlHandle(), FALSE, &contentRect);
        DrawEdge(hdcMem, &contentRect, EDGE_SUNKEN, BF_RECT);

        // Copy the tab control from memory DC to the screen
        BitBlt(hdc, 0, 0, tabRect.right, tabRect.bottom, hdcMem, 0, 0, SRCCOPY);

        // Clean up tab control painting resources
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
        return 0;
    }


