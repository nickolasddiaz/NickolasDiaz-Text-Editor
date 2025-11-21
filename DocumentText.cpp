#include <Windows.h>
#include <cstdlib>
#include <utility>
#include <vector>
#include <string>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#include <memory>
#include <stack>

#include "DocumentText.h"



DocumentText::DocumentText(HWND parentWindow)
    : textboxhwnd(parentWindow), buffer(nullptr), bufferSize(0),
          gapStart(0), gapEnd(0), gapSize(0) {}

DocumentText::~DocumentText() {
    delete[] buffer;
}

bool DocumentText::initFile(const wchar_t* filename) {
    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
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
    const DWORD fileSize = GetFileSize(hFile, nullptr);
    if (fileSize == INVALID_FILE_SIZE) {
        return false;
    }

    bufferSize = fileSize + 1024;
    buffer = new char[bufferSize];


    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, nullptr) || bytesRead != fileSize) {
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

void DocumentText::getText(const size_t pos, const size_t len, char* temp) const {
    if (pos >= getLength() || len == 0) {
        temp[0] = '\0';
        return;
    }

    const size_t end = std::min(pos + len, getLength());
    const size_t actualLen = end - pos;

    if (pos < gapStart) {
        const size_t beforeGap = std::min(actualLen, gapStart - pos);
        memcpy(temp, buffer + pos, beforeGap);

        if (beforeGap < actualLen) {
            size_t afterGap = actualLen - beforeGap;
            memcpy(temp + beforeGap, buffer + gapEnd, afterGap);
        }
    }
    else {
        memcpy(temp, buffer + pos + gapSize, actualLen);
    }

    temp[actualLen] = '\0';
}

void DocumentText::insertText(const char* text, size_t len, size_t position) {
    if (buffer == nullptr) {
        bufferSize = std::max(len + 1024, static_cast<size_t>(1024));
        buffer = new char[bufferSize];
        gapStart = 0;
        gapEnd = bufferSize;
        gapSize = bufferSize;
    }

    moveGap(position);

    // Ensure that the gap can accommodate the new text.
    while (len > gapSize) {
        expandBuffer();
    }

    // Perform the copy only when the gap size is sufficient.
    if (len <= gapSize) {
        memcpy(buffer + gapStart, text, len);
        gapStart += len;
        gapSize -= len;
    }

    updateLineStarts();
}


void DocumentText::deleteText(size_t start, size_t end) {
    if (start >= end || start >= getLength() || end > getLength()) {
        return;
    }

    moveGap(start);
    const size_t deleteSize = end - start;
    gapEnd = std::min(gapEnd + deleteSize, bufferSize);
    gapSize = gapEnd - gapStart;

    updateLineStarts();
}

size_t DocumentText::getLength() const {
    return bufferSize - gapSize;
}

void DocumentText::moveGap(size_t position) {
    if (position == gapStart)
        return;

    // Clamp logical position to valid text range
    if (const size_t textLength = getLength(); position > textLength)
        position = textLength;

    if (position < gapStart) {
        // Move gap left
        const size_t moveSize = gapStart - position;
        memmove(buffer + gapEnd - moveSize,
                buffer + position,
                moveSize);
        gapStart -= moveSize;
        gapEnd   -= moveSize;
    } else {
        // Move gap right
        size_t moveSize = position - gapStart;

        // moveSize bytes physically begin at gapEnd
        memmove(buffer + gapStart,
                buffer + gapEnd,
                moveSize);

        gapStart += moveSize;
        gapEnd   += moveSize;
    }
}

void DocumentText::expandBuffer() {
    size_t newSize = bufferSize * 2;
    char* newBuffer = new char[newSize];

    // Copy content before gap
    memcpy(newBuffer, buffer, gapStart);

    // Copy content after gap
    const size_t afterGapSize = bufferSize - gapEnd;
    memcpy(newBuffer + newSize - afterGapSize, buffer + gapEnd, afterGapSize);

    delete[] buffer;
    buffer = newBuffer;
    gapEnd = newSize - afterGapSize;
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

ULONG DocumentText::get_line(ULONG lineno, char* buf, size_t len) const {
    if (lineno >= lineStarts.size()) {
        return 0;
    }
    size_t lineStart = lineStarts[lineno];
    size_t lineEnd = (lineno + 1 < lineStarts.size()) ? lineStarts[lineno + 1] - 1 : getLength();
    size_t lineLength = lineEnd - lineStart;

    if (lineStart < gapStart) {
        lineLength = std::min(lineLength, len);
        memcpy(buf, buffer + lineStart, lineLength);
    }
    else {
        lineStart += gapSize;
        lineLength = std::min(lineLength, len);
        memcpy(buf, buffer + lineStart, lineLength);
    }
    return lineLength;
}


void DocumentText::setCaretPosition(size_t position) const {
    // Set the caret position using Windows API
    SendMessage(textboxhwnd, EM_SETSEL, (WPARAM)position, static_cast<LPARAM>(position));
}

size_t DocumentText::getCaretPosition() const {
    // Get the current caret position using Windows API
    DWORD startPos, endPos;
    SendMessage(textboxhwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&startPos), reinterpret_cast<LPARAM>(&endPos));
    return startPos;
}


    InsertCommand::InsertCommand(DocumentText& buf, std::string  t, size_t pos)
        : buffer(buf), text(std::move(t)), position(pos) {}

    void InsertCommand::execute()  {
        buffer.insertText(text.c_str(), text.length(), position);
    }

    void InsertCommand::undo()  {
        buffer.deleteText(position, position + text.length());
    }

    size_t InsertCommand::getCursorPosition() const {
        return position + text.length(); // After insertion
    }

    size_t InsertCommand::getUndoCursorPosition() const {
        return position;
    }



    DeleteCommand::DeleteCommand(DocumentText& buf, size_t pos, size_t len)
        : buffer(buf), position(pos) {
        char* temp = new char[len + 1];
        buffer.getText(pos, len, temp);
        deletedText = std::string(temp, len);
        delete[] temp;
    }

    void DeleteCommand::execute()  {
        buffer.deleteText(position, position + deletedText.length());
    }

    void DeleteCommand::undo()  {
        buffer.insertText(deletedText.c_str(), deletedText.length(), position);
    }

    size_t DeleteCommand::getCursorPosition() const  {
        return position; // After deletion
    }

    size_t DeleteCommand::getUndoCursorPosition() const {
        return position + deletedText.length();
    }

    void CommandHistory::executeCommand(std::unique_ptr<Command> cmd) {
        cmd->execute();
        lastCursorPosition = cmd->getCursorPosition();
        undoStack.push(std::move(cmd));
        // Clear redo stack
        while (!redoStack.empty()) {
            redoStack.pop();
        }
    }

    void CommandHistory::undo() {
        if (!undoStack.empty()) {
            auto cmd = std::move(undoStack.top());
            undoStack.pop();
            cmd->undo();
            lastCursorPosition = cmd->getUndoCursorPosition();
            redoStack.push(std::move(cmd));
        }
    }

    void CommandHistory::redo() {
        if (!redoStack.empty()) {
            auto cmd = std::move(redoStack.top());
            redoStack.pop();
            cmd->execute();
            lastCursorPosition = cmd->getCursorPosition();
            undoStack.push(std::move(cmd));
        }
    }

    size_t CommandHistory::getLastCursorPosition() const {
        return lastCursorPosition;
    }

