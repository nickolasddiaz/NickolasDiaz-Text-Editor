#ifndef DOCUMENTTEXT_H
#define DOCUMENTTEXT_H

#include <Windows.h>
#include <stdlib.h>
#include <commctrl.h>
#include <string>

class DocumentText {
public:
    DocumentText(HWND parentWindow);
    ~DocumentText();

    bool initFile(const wchar_t* filename);
    bool initHandle(HANDLE hFile);
    ULONG getline(ULONG lineno, char* buf, size_t len);
    void insertText(const char* text, size_t len, size_t position);
    void deleteText(size_t start, size_t end);
    void moveGap(size_t position);
    size_t getLength() const { return bufferSize - gapSize; }
    std::vector<size_t> lineStarts;
    void updateLineStarts();

private:
    HWND textboxhwnd;
    char* buffer;
    size_t bufferSize = 1024;
    size_t gapStart;
    size_t gapEnd;
    size_t gapSize;

    void expandBuffer();
    
};

#endif // DOCUMENTTEXT_H