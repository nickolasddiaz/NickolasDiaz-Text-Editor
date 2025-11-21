#include "TextEditor.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

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


void TextEditor::show() const {
    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);
}

LRESULT CALLBACK TextEditor::WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (auto* pThis = reinterpret_cast<TextEditor*>(GetWindowLongPtr(hWnd, GWLP_USERDATA))) {
        return pThis->handleMessage(hWnd, msg, wp, lp);
    }
    else if (msg == WM_NCCREATE) {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lp);
        pThis = static_cast<TextEditor*>(pCreate->lpCreateParams);
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
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
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
        nullptr, nullptr, hInstance, this
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
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_EXIT, L"Exit");

    AppendMenu(hEditMenu, MF_STRING, EDIT_MENU_UNDO, L"Undo\tCtrl+Z");
    AppendMenu(hEditMenu, MF_STRING, EDIT_MENU_REDO, L"Redo\tCtrl+Y");


    AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"File");
    AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hEditMenu), L"Edit");

    SetMenu(hMainWindow, hMenu);
}

void TextEditor::addControls() {
    tabControl = new TabControl(hMainWindow);

    HWND hEditFile = CreateWindowW(
        L"EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
        0, 30, 700, 670,
        hMainWindow, nullptr, GetModuleHandle(nullptr), nullptr
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
            auto* nmhdr = reinterpret_cast<NMHDR *>(lp);
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
                default: ;
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

void TextEditor::handleException(const std::exception& e) const {
    std::wstring errorMessage = L"An error occurred: ";
    errorMessage += std::wstring(e.what(), e.what() + strlen(e.what()));
    MessageBoxW(hMainWindow, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
}

void TextEditor::handleUnknownException() const {
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
    default: ;
    }
    return 0;
}

void TextEditor::createNewTab() {
    RECT rcClient;
    GetClientRect(hMainWindow, &rcClient);

    HWND newEditFile = CreateWindowW(
        L"EDIT", nullptr,
        WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
        0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
        hMainWindow, nullptr, GetModuleHandle(nullptr), nullptr
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
        if (fileName != nullptr) {
            fileName++;
        }
        else {
            fileName = filePath;
        }

        HWND newEditFile = CreateWindowW(
            L"EDIT", nullptr,
            WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
            0, 30, 700, 670,
            hMainWindow, nullptr, GetModuleHandle(nullptr), nullptr
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


void TextEditor::saveFile() const {
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

void TextEditor::saveFileAs() const {
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
        if (fileName != nullptr) {
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
            InvalidateRect(hMainWindow, nullptr, TRUE);
        }
    }
}

void TextEditor::saveAllFiles() const {
    for (int i = 0; i < tabControl->getTabCount(); ++i) {
        tabControl->setCurrentTab(i);
        saveFile();
    }
}

void TextEditor::displayFile(const DocumentText* document, HWND editControl) {
    // Get the full document content directly from the buffer
    size_t totalLen = document->getLength();

    if (totalLen == 0) {
        SetWindowTextW(editControl, L"");
        return;
    }

    // Allocate buffer and get all text
    char* content = new char[totalLen + 1];
    document->getText(0, totalLen, content);

    // Convert LF to CRLF for the Edit control (if needed)
    std::string result;
    result.reserve(totalLen * 2);  // Worst case: every char is LF

    for (size_t i = 0; i < totalLen; ++i) {
        if (content[i] == '\n' && (i == 0 || content[i-1] != '\r')) {
            result += '\r';
        }
        result += content[i];
    }

    delete[] content;

    // Convert to wide string
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, result.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wideBuffer(wideSize);
    MultiByteToWideChar(CP_UTF8, 0, result.c_str(), -1, wideBuffer.data(), wideSize);

    SetWindowTextW(editControl, wideBuffer.data());
}

void TextEditor::writeFile(const std::wstring& path, const DocumentText* document) const {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxW(hMainWindow, L"Failed to save file", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    char buf[LONGEST_LINE];
    for (size_t i = 0; i < document->lineStarts.size(); ++i) {
        ULONG len = document->get_line(i, buf, LONGEST_LINE);
        DWORD bytesWritten;
        WriteFile(hFile, buf, len, &bytesWritten, nullptr);
        if (i < document->lineStarts.size() - 1) {  // Don't add newline after the last line
            WriteFile(hFile, "\r\n", 2, &bytesWritten, nullptr);
        }
    }

    CloseHandle(hFile);
}

void TextEditor::updateWindowTitle() const {
    std::wstring currentFilePath = tabControl->getCurrentFilePath();
    std::wstring title = L"Nickolas Text Editor - " + (currentFilePath.empty() ? L"Untitled" : currentFilePath);
    SetWindowTextW(hMainWindow, title.c_str());
}

LRESULT TextEditor::OnPaint(HWND hWnd) const {
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


LONG TextEditor::PaintLine(HDC hdc, ULONG nLineNo, const DocumentText* document, const RECT& clientRect) const {
    char buf[LONGEST_LINE];

    RECT rect = clientRect;
    rect.top = nLineNo * m_nFontHeight;
    rect.bottom = rect.top + m_nFontHeight;

    ULONG len = document->get_line(nLineNo, buf, LONGEST_LINE);

    TCHAR wbuf[LONGEST_LINE];
    MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, LONGEST_LINE);

    ExtTextOutW(hdc, rect.left, rect.top, ETO_OPAQUE, &rect, wbuf, len, nullptr);

    return 0;
}
LRESULT CALLBACK TextEditor::SubclassEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    auto* pThis = reinterpret_cast<TextEditor*>(dwRefData);

    switch (uMsg) {
    case WM_CHAR: {
        if (wParam >= 32 || wParam == VK_TAB || wParam == VK_RETURN) {
            DWORD start, end;
            SendMessage(hWnd, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));

            // If there's a selection, delete it first
            if (start != end) {
                pThis->commandHistory.executeCommand(
                    std::make_unique<DeleteCommand>(*(pThis->getCurrentDocument()), start, end - start));
            }

            char ch = static_cast<char>(wParam);
            std::string insertText(1, ch);
            pThis->commandHistory.executeCommand(
                std::make_unique<InsertCommand>(*(pThis->getCurrentDocument()), insertText, start));

            // Let default proc handle the visual update
            LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

            // Clear the Edit control's internal undo buffer to prevent conflicts
            SendMessage(hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
            return result;
        }
        break;
    }
    case WM_KEYDOWN: {
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            switch (wParam) {
            case 'Z':
                pThis->undo();
                return 0;
            case 'Y':
                pThis->redo();
                return 0;
            default:
                break;
            }
        }

        if (wParam == VK_BACK || wParam == VK_DELETE) {
            DWORD start, end;
            SendMessage(hWnd, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));

            if (start == end) {
                if (wParam == VK_BACK && start > 0) {
                    --start;
                }
                else if (wParam == VK_DELETE && end < pThis->getCurrentDocument()->getLength()) {
                    ++end;
                }
            }

            if (start != end) {
                pThis->commandHistory.executeCommand(
                    std::make_unique<DeleteCommand>(*(pThis->getCurrentDocument()), start, end - start));
            }

            LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            SendMessage(hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
            return result;
        }
        break;
    }
    case WM_PASTE: {
        if (OpenClipboard(hWnd)) {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData != nullptr) {
                char* pszText = static_cast<char*>(GlobalLock(hData));
                if (pszText != nullptr) {
                    DWORD start, end;
                    SendMessage(hWnd, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));

                    // Delete selection first if any
                    if (start != end) {
                        pThis->commandHistory.executeCommand(
                            std::make_unique<DeleteCommand>(*(pThis->getCurrentDocument()), start, end - start));
                    }

                    pThis->commandHistory.executeCommand(
                        std::make_unique<InsertCommand>(*(pThis->getCurrentDocument()), pszText, start));

                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        SendMessage(hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
        return result;
    }
    case WM_CUT: {
        DWORD start, end;
        SendMessage(hWnd, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));

        if (start != end) {
            pThis->commandHistory.executeCommand(
                std::make_unique<DeleteCommand>(*(pThis->getCurrentDocument()), start, end - start));
        }

        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        SendMessage(hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
        return result;
    }
    default:
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

DocumentText* TextEditor::getCurrentDocument() const {
    int currentTabIndex = tabControl->getCurrentTabIndex();
    if (currentTabIndex >= 0 && currentTabIndex < documents.size()) {
        return documents[currentTabIndex].get();
    }
    return nullptr;
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

void TextEditor::setCursorPosition(size_t position) const {
    HWND currentEditControl = tabControl->getCurrentEditControl();
    SendMessage(currentEditControl, EM_SETSEL, position, position);
    SendMessage(currentEditControl, EM_SCROLLCARET, 0, 0);
}

void TextEditor::updateEditControl() const {
    int currentTabIndex = tabControl->getCurrentTabIndex();
    if (currentTabIndex >= 0 && currentTabIndex < documents.size()) {
        DocumentText* currentDoc = documents[currentTabIndex].get();
        HWND editControl = tabControl->getCurrentEditControl();
        displayFile(currentDoc, editControl);
    }
}
