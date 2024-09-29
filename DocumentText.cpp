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

#include "DocumentText.h"



DocumentText::DocumentText(HWND parentWindow)
    : textboxhwnd(parentWindow), buffer(nullptr), bufferSize(0),
    gapStart(0), gapEnd(0), gapSize(0) {}

DocumentText::~DocumentText() {
    delete[] buffer;
}

bool DocumentText::initFile(const wchar_t* filename) {
    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxW(textboxhwnd, L"Failed to open file", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    bool result = initHandle(hFile);
    CloseHandle(hFile);
    if (!result) {
        MessageBoxW(textboxhwnd, L"Failed to initialize file buffer", L"Error", MB_OK | MB_ICONERROR);
    }
    return result;
}

bool DocumentText::initHandle(HANDLE hFile) {
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        return false;
    }

    bufferSize = fileSize + 1024;
    buffer = new char[bufferSize];
    if (!buffer) {
        return false;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        delete[] buffer;
        buffer = nullptr;
        return false;
    }

    gapStart = fileSize;
    gapEnd = bufferSize;
    gapSize = gapEnd - gapStart;

    updateLineStarts();
    return true;
}

char* buffer;
int length;
ULONG numlines;
ULONG* linebuffer;
HWND textboxhwnd;

void DocumentText::insertText(const char* text, size_t len, size_t position) {
    if (buffer == NULL) {
        bufferSize = 1024;
        buffer = new char[bufferSize];
        gapEnd = bufferSize;
        gapSize = bufferSize;
    }

    moveGap(position);
    if (len > gapSize) {
        expandBuffer();
    }
    memcpy(buffer + gapStart, text, len);
    gapStart += len;
    gapSize -= len;
    updateLineStarts();
}

void DocumentText::deleteText(size_t start, size_t end) {
    moveGap(start);
    size_t deleteSize = end - start;
    gapEnd += deleteSize;
    gapSize += deleteSize;
    updateLineStarts();
}

void DocumentText::moveGap(size_t position) {
    if (position == gapStart) {
        return;
    }
    if (position < gapStart) {
        size_t moveSize = gapStart - position;
        memmove(buffer + gapEnd - moveSize, buffer + position, moveSize);
        gapEnd -= moveSize;
        gapStart -= moveSize;
    }
    else {
        size_t moveSize = position - gapStart;
        memmove(buffer + gapStart, buffer + gapEnd, moveSize);
        gapStart += moveSize;
        gapEnd += moveSize;
    }
}
void DocumentText::expandBuffer() {
    size_t newSize = bufferSize * 2;
    char* newBuffer = new char[newSize];

    memcpy(newBuffer, buffer, gapStart);
    memcpy(newBuffer + newSize - (bufferSize - gapEnd), buffer + gapEnd, bufferSize - gapEnd);
    delete[] buffer;
    buffer = newBuffer;
    gapEnd = newSize - (bufferSize - gapEnd);
    gapSize = gapEnd - gapStart;
    bufferSize = newSize;
}

void DocumentText::updateLineStarts() {
    lineStarts.clear();
    lineStarts.push_back(0);
    for (size_t i = 0; i < gapStart; ++i) {
        if (buffer[i] == '\n') {
            lineStarts.push_back(i + 1);
        }
    }
    for (size_t i = gapEnd; i < bufferSize; ++i) {
        if (buffer[i] == '\n') {
            lineStarts.push_back(i - gapSize + 1);
        }
    }
}

ULONG DocumentText::getline(ULONG lineno, char* buf, size_t len) {
    if (lineno >= lineStarts.size()) {
        return 0;
    }
    size_t lineStart = lineStarts[lineno];
    size_t lineEnd = (lineno + 1 < lineStarts.size()) ? lineStarts[lineno + 1] - 1 : getLength();
    size_t lineLength = lineEnd - lineStart;

    if (lineStart < gapStart) {
        lineLength = min(lineLength, len);
        memcpy(buf, buffer + lineStart, lineLength);
    }
    else {
        lineStart += gapSize;
        lineLength = min(lineLength, len);
        memcpy(buf, buffer + lineStart, lineLength);
    }
    return static_cast<ULONG>(lineLength);
}