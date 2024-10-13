#include "TextEditor.h"
#include <commctrl.h>
#include <windowsx.h>

constexpr int FILE_MENU_NEW = 1;
constexpr int FILE_MENU_OPEN = 2;
constexpr int FILE_MENU_EXIT = 3;
constexpr int FILE_MENU_SAVE = 4;
constexpr int FILE_MENU_SAVE_AS = 5;
constexpr int FILE_MENU_SAVE_ALL = 6;

constexpr int EDIT_MENU_UNDO = 101;
constexpr int EDIT_MENU_REDO = 102;




TextEditor::TextEditor() {
    registerWindowClass();
    createMainWindow();
    addMenus();
    addControls();
    m_nFontHeight = 16;
    tabControl->onTabRemoved = [this](int index) {
        if (index >= 0 && index < documents.size()) {
            // Erase the document at the given index
            documents.erase(documents.begin() + index);

            // Ensure that the current tab index is within bounds
            if (tabControl->getCurrentTabIndex() >= documents.size()) {
                tabControl->setCurrentTab(documents.size() - 1);  // Set to last tab if out of bounds
            }
        }
        };

}


void TextEditor::show() {
    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);
}

LRESULT CALLBACK TextEditor::WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    TextEditor* pThis = reinterpret_cast<TextEditor*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (pThis) {
        return pThis->handleMessage(hWnd, msg, wp, lp);
    }
    else if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lp);
        pThis = reinterpret_cast<TextEditor*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        return pThis->handleMessage(hWnd, msg, wp, lp);
    }

    return DefWindowProc(hWnd, msg, wp, lp);
}

void TextEditor::setInstance(HINSTANCE hInst) {
    hInstance = hInst;
}

void TextEditor::registerWindowClass() {
    WNDCLASSW wc = { 0 };
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TextEditorClass";
    wc.lpfnWndProc = WindowProcedure;
    RegisterClassW(&wc);
}

void TextEditor::createMainWindow() {
    hMainWindow = CreateWindowW(
        L"TextEditorClass", L"Nickolas Text Editor - new file",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        0, 300, 700, 700,
        NULL, NULL, hInstance, this
    );
}

void TextEditor::addMenus() {
    hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hEditMenu = CreateMenu();

    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_NEW, L"New File");
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_OPEN, L"Open File");
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_SAVE, L"Save");
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_SAVE_AS, L"Save As");
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_SAVE_ALL, L"Save All");
    AppendMenu(hFileMenu, MF_SEPARATOR, NULL, NULL);
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_EXIT, L"Exit");

    AppendMenu(hEditMenu, MF_STRING, EDIT_MENU_UNDO, L"Undo\tCtrl+Z");
    AppendMenu(hEditMenu, MF_STRING, EDIT_MENU_REDO, L"Redo\tCtrl+Y");


    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hEditMenu, L"Edit");

    SetMenu(hMainWindow, hMenu);
}

void TextEditor::addControls() {
    tabControl = new TabControl(hMainWindow);

    HWND hEditFile = CreateWindowW(
        L"EDIT", NULL,
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
        0, 30, 700, 670,
        hMainWindow, NULL, GetModuleHandle(NULL), NULL
    );

    documents.push_back(std::make_unique<DocumentText>(hEditFile));
    tabControl->addTab(L"Untitled", hEditFile);
    SubclassEditControl(hEditFile);  // Apply subclassing to the first edit control
}

void TextEditor::SubclassEditControl(HWND hEditControl) {
    SetWindowSubclass(hEditControl, SubclassEditProc, 0, reinterpret_cast<DWORD_PTR>(this));
}

LRESULT TextEditor::handleMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    try {
        switch (msg) {
        case WM_COMMAND:
            return handleCommand(wp, lp);

        case WM_NOTIFY: {
            NMHDR* nmhdr = (NMHDR*)lp;
            if (nmhdr->hwndFrom == tabControl->getTabControlHandle() && nmhdr->code == TCN_SELCHANGE) {
                int selectedTabIndex = tabControl->getCurrentTabIndex();
                tabControl->setCurrentTab(selectedTabIndex);
            }
            break;
        }

        case WM_SIZE:
            if (tabControl) {
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);
                tabControl->Resize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            return OnPaint(hWnd);

        case WM_KEYDOWN:
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                switch (wp) {
                case 'Z':
                    undo();
                    return 0;
                case 'Y':
                    redo();
                    return 0;
                }
            }
            break;
        default:
            return DefWindowProc(hWnd, msg, wp, lp);
        }
        return 0;
    }
    catch (const std::exception& e) {
        handleException(e);
        return 0;
    }
    catch (...) {
        handleUnknownException();
        return 0;
    }
}

void TextEditor::handleException(const std::exception& e) {
    std::wstring errorMessage = L"An error occurred: ";
    errorMessage += std::wstring(e.what(), e.what() + strlen(e.what()));
    MessageBoxW(hMainWindow, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
}

void TextEditor::handleUnknownException() {
    MessageBoxW(hMainWindow, L"An unknown error occurred.", L"Error", MB_OK | MB_ICONERROR);
}


LRESULT TextEditor::handleCommand(WPARAM wp, LPARAM lp) {
    switch (wp) {
    case FILE_MENU_NEW:
        createNewTab();
        break;
    case FILE_MENU_OPEN:
        openFile();
        break;
    case FILE_MENU_SAVE:
        saveFile();
        break;
    case FILE_MENU_SAVE_AS:
        saveFileAs();
        break;
    case FILE_MENU_SAVE_ALL:
        saveAllFiles();
        break;
    case FILE_MENU_EXIT:
        if (MessageBoxW(hMainWindow, L"Are you sure?", L"Exit", MB_YESNO | MB_ICONWARNING) == IDYES) {
            DestroyWindow(hMainWindow);
        }
        break;
    case EDIT_MENU_UNDO:
        undo();
        return 0;
    case EDIT_MENU_REDO:
        redo();
        return 0;
    }
    return 0;
}

void TextEditor::createNewTab() {
    RECT rcClient;
    GetClientRect(hMainWindow, &rcClient);

    HWND newEditFile = CreateWindowW(
        L"EDIT", NULL,
        WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
        0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
        hMainWindow, NULL, GetModuleHandle(NULL), NULL
    );

    documents.push_back(std::make_unique<DocumentText>(newEditFile));
    tabControl->addTab(L"Untitled", newEditFile);
    SubclassEditControl(newEditFile);
    tabControl->setCurrentTab(tabControl->getTabCount() - 1);

    RECT rcMain;
    GetClientRect(hMainWindow, &rcMain);
    tabControl->Resize(rcMain.right - rcMain.left, rcMain.bottom - rcMain.top);
}

void TextEditor::openFile() {
    OPENFILENAMEW ofn = { 0 };
    wchar_t filePath[MAX_PATH] = L"";

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;

    if (GetOpenFileNameW(&ofn)) {
        wchar_t* fileName = wcsrchr(filePath, L'\\');
        if (fileName != NULL) {
            fileName++;
        }
        else {
            fileName = filePath;
        }

        HWND newEditFile = CreateWindowW(
            L"EDIT", NULL,
            WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
            0, 30, 700, 670,
            hMainWindow, NULL, GetModuleHandle(NULL), NULL
        );

        // Initialize the document
        auto newDocument = std::make_unique<DocumentText>(newEditFile);
        if (newDocument->initFile(filePath)) {
            documents.push_back(std::move(newDocument));
            tabControl->addTab(fileName, newEditFile, filePath);
            SubclassEditControl(newEditFile);
            tabControl->setCurrentTab(tabControl->getTabCount() - 1);
            displayFile(documents.back().get(), newEditFile);
            updateWindowTitle();
        }
        else {
            // Handle failure to initialize document
            DestroyWindow(newEditFile);  // Cleanup the edit control if file load failed
            MessageBoxW(hMainWindow, L"Failed to open file", L"Error", MB_OK | MB_ICONERROR);
        }

    }
}


void TextEditor::saveFile() {
    std::wstring currentFilePath = tabControl->getCurrentFilePath();
    if (currentFilePath.empty()) {
        saveFileAs();
    }
    else {
        int currentTabIndex = tabControl->getCurrentTabIndex();
        if (currentTabIndex >= 0 && currentTabIndex < documents.size()) {
            writeFile(currentFilePath, documents[currentTabIndex].get());

        }
    }
}

void TextEditor::saveFileAs() {
    OPENFILENAMEW ofn = { 0 };
    wchar_t filePath[MAX_PATH] = L"";

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetSaveFileNameW(&ofn)) {
        wchar_t* fileName = wcsrchr(filePath, L'\\');
        if (fileName != NULL) {
            fileName++;
        }
        else {
            fileName = filePath;
        }

        int currentTabIndex = tabControl->getCurrentTabIndex();
        if (currentTabIndex >= 0 && currentTabIndex < documents.size()) {
            writeFile(filePath, documents[currentTabIndex].get());
            tabControl->setCurrentFilePath(filePath);
            updateWindowTitle();
            tabControl->changeTabName(fileName);
            InvalidateRect(hMainWindow, NULL, TRUE);
        }
    }
}

void TextEditor::saveAllFiles() {
    for (int i = 0; i < tabControl->getTabCount(); ++i) {
        tabControl->setCurrentTab(i);
        saveFile();
    }
}

void TextEditor::displayFile(DocumentText* document, HWND editControl) {
    std::string content;
    char buf[LONGEST_LINE];
    for (size_t i = 0; i < document->lineStarts.size(); ++i) {
        ULONG len = document->getline(i, buf, LONGEST_LINE);
        content.append(buf, len);
        content.append("\r\n");
    }

    // Convert UTF-8 to wide string
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wideBuffer(wideSize);
    MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, wideBuffer.data(), wideSize);

    SetWindowTextW(editControl, wideBuffer.data());
}

void TextEditor::writeFile(const std::wstring& path, DocumentText* document) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxW(hMainWindow, L"Failed to save file", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    char buf[LONGEST_LINE];
    for (size_t i = 0; i < document->lineStarts.size(); ++i) {
        ULONG len = document->getline(i, buf, LONGEST_LINE);
        DWORD bytesWritten;
        WriteFile(hFile, buf, len, &bytesWritten, NULL);
        if (i < document->lineStarts.size() - 1) {  // Don't add newline after the last line
            WriteFile(hFile, "\r\n", 2, &bytesWritten, NULL);
        }
    }

    CloseHandle(hFile);
}

void TextEditor::updateWindowTitle() {
    std::wstring currentFilePath = tabControl->getCurrentFilePath();
    std::wstring title = L"Nickolas Text Editor - " + (currentFilePath.empty() ? L"Untitled" : currentFilePath);
    SetWindowTextW(hMainWindow, title.c_str());
}

LRESULT TextEditor::OnPaint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);


    // Paint the document content
    int currentTab = tabControl->getCurrentTabIndex();
    if (currentTab >= 0 && currentTab < documents.size()) {
        DocumentText* currentDoc = documents[currentTab].get();
        RECT rcClient;
        GetClientRect(tabControl->getCurrentEditControl(), &rcClient);


        int first = ps.rcPaint.top / m_nFontHeight;
        int last = ps.rcPaint.bottom / m_nFontHeight;

        for (ULONG i = first; i <= last; i++) {
            PaintLine(hdc, i, currentDoc, rcClient);
        }
    }

    EndPaint(hWnd, &ps);
    return 0;
}


LONG TextEditor::PaintLine(HDC hdc, ULONG nLineNo, DocumentText* document, const RECT& clientRect) {
    char buf[LONGEST_LINE];
    ULONG len;

    RECT rect = clientRect;
    rect.top = nLineNo * m_nFontHeight;
    rect.bottom = rect.top + m_nFontHeight;

    len = document->getline(nLineNo, buf, LONGEST_LINE);

    TCHAR wbuf[LONGEST_LINE];
    MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, LONGEST_LINE);

    ExtTextOutW(hdc, rect.left, rect.top, ETO_OPAQUE, &rect, wbuf, len, NULL);

    return 0;
}
LRESULT CALLBACK TextEditor::SubclassEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    TextEditor* pThis = reinterpret_cast<TextEditor*>(dwRefData);

    switch (uMsg) {
    case WM_CHAR: {
        if (wParam >= 32 || wParam == VK_TAB || wParam == VK_RETURN) {
            // Get current selection
            DWORD start, end;
            SendMessage(hWnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

            // Create and execute insert command
            char ch = static_cast<char>(wParam);
            std::string insertText(1, ch);
            pThis->commandHistory.executeCommand(std::make_unique<InsertCommand>(*(pThis->getCurrentDocument()), insertText, start));

            // Let the default proc handle the actual insertion
            LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            pThis->SynchronizeDocumentWithEdit(hWnd);
            return result;
        }
        break;
    }
    case WM_KEYDOWN: {
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            switch (wParam) {
            case 'Z':
                pThis->undo();
                return 0; // Prevent default handling
            case 'Y':
                pThis->redo();
                return 0; // Prevent default handling
            }
        }

        if (wParam == VK_BACK || wParam == VK_DELETE) {
            // Get current selection
            DWORD start, end;
            SendMessage(hWnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

            if (start == end) {
                if (wParam == VK_BACK && start > 0) {
                    --start;
                }
                else if (wParam == VK_DELETE && end < pThis->getCurrentDocument()->getLength()) {
                    ++end;
                }
            }

            if (start != end) {
                // Create and execute delete command
                pThis->commandHistory.executeCommand(std::make_unique<DeleteCommand>(*(pThis->getCurrentDocument()), start, end - start));
            }

            // Let the default proc handle the actual deletion
            LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            pThis->SynchronizeDocumentWithEdit(hWnd);
            return result;
        }
        break;
    }
    case WM_PASTE: {
        // Handle paste operation
        if (OpenClipboard(hWnd)) {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData != NULL) {
                char* pszText = static_cast<char*>(GlobalLock(hData));
                if (pszText != NULL) {
                    // Get current selection
                    DWORD start, end;
                    SendMessage(hWnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

                    // Create and execute insert command
                    pThis->commandHistory.executeCommand(std::make_unique<InsertCommand>(*(pThis->getCurrentDocument()), pszText, start));

                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
        // Let the default proc handle the actual paste
        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        pThis->SynchronizeDocumentWithEdit(hWnd);
        return result;
    }
    case WM_CUT: {
        // Handle cut operation
        // Get current selection
        DWORD start, end;
        SendMessage(hWnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

        if (start != end) {
            // Create and execute delete command
            pThis->commandHistory.executeCommand(std::make_unique<DeleteCommand>(*(pThis->getCurrentDocument()), start, end - start));
        }

        // Let the default proc handle the actual cut
        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        pThis->SynchronizeDocumentWithEdit(hWnd);
        return result;
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

DocumentText* TextEditor::getCurrentDocument() {
    int currentTabIndex = tabControl->getCurrentTabIndex();
    if (currentTabIndex >= 0 && currentTabIndex < documents.size()) {
        return documents[currentTabIndex].get();
    }
    return nullptr;
}

void TextEditor::SynchronizeDocumentWithEdit(HWND hEditControl) {
    int currentTabIndex = tabControl->getCurrentTabIndex();
    if (currentTabIndex < 0 || currentTabIndex >= documents.size()) {
        return;
    }

    DocumentText* currentDoc = documents[currentTabIndex].get();

    // Get the entire text from the edit control
    int textLength = GetWindowTextLength(hEditControl);
    std::vector<wchar_t> buffer(textLength + 1);
    GetWindowText(hEditControl, buffer.data(), textLength + 1);

    // Convert wide string to UTF-8
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> utf8Buffer(utf8Length);
    WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1, utf8Buffer.data(), utf8Length, nullptr, nullptr);

    // Clear the current document content
    currentDoc->deleteText(0, currentDoc->getLength());

    // Insert the new content
    currentDoc->insertText(utf8Buffer.data(), utf8Length - 1, 0);  // -1 to exclude null terminator
}

void TextEditor::undo() {
    size_t currentPosition = getCurrentDocument()->getCaretPosition();
    commandHistory.undo();
    updateEditControl();

    size_t newPosition = commandHistory.getLastCursorPosition();
    if (newPosition == 0) {
        newPosition = currentPosition;
    }
    setCursorPosition(newPosition);
}

void TextEditor::redo() {
    size_t currentPosition = getCurrentDocument()->getCaretPosition();
    commandHistory.redo();
    updateEditControl();

    size_t newPosition = commandHistory.getLastCursorPosition();
    if (newPosition == 0) {
        newPosition = currentPosition;
    }
    setCursorPosition(newPosition);
}

void TextEditor::setCursorPosition(size_t position) {
    HWND currentEditControl = tabControl->getCurrentEditControl();
    SendMessage(currentEditControl, EM_SETSEL, position, position);
    SendMessage(currentEditControl, EM_SCROLLCARET, 0, 0);
}

void TextEditor::updateEditControl() {
    int currentTabIndex = tabControl->getCurrentTabIndex();
    if (currentTabIndex >= 0 && currentTabIndex < documents.size()) {
        DocumentText* currentDoc = documents[currentTabIndex].get();
        HWND editControl = tabControl->getCurrentEditControl();
        displayFile(currentDoc, editControl);
    }
}