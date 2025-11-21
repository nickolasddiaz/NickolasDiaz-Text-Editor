#ifndef DOCUMENTTEXT_H
#define DOCUMENTTEXT_H

#include <Windows.h>
#include <string>
#include <stack>


class DocumentText {
public:
    void setCaretPosition(size_t position) const;
    [[nodiscard]] size_t getCaretPosition() const;

    explicit DocumentText(HWND parentWindow);
    ~DocumentText();

    bool initFile(const wchar_t* filename);
    bool initHandle(HANDLE hFile);
    ULONG get_line(ULONG lineno, char* buf, size_t len) const;
    void insertText(const char* text, size_t len, size_t position);
    void deleteText(size_t start, size_t end);
    void moveGap(size_t position);
    [[nodiscard]] size_t getLength() const;
    std::vector<size_t> lineStarts;
    void updateLineStarts();
    void getText(size_t pos, size_t len, char* temp) const;


private:
    HWND textboxhwnd;
    char* buffer;
    size_t bufferSize = 1024;
    size_t gapStart;
    size_t gapEnd;
    size_t gapSize;
    void expandBuffer();
    
};

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    [[nodiscard]] virtual size_t getCursorPosition() const = 0;

    [[nodiscard]] virtual size_t getUndoCursorPosition() const = 0;
};

class InsertCommand : public Command{
    DocumentText & buffer;
    std::string text;
    size_t position;

public:
    InsertCommand(DocumentText& buf, std::string  t, size_t pos);

    void execute() override;
    [[nodiscard]] size_t getCursorPosition() const override;

    [[nodiscard]] size_t getUndoCursorPosition() const override;

    void undo() override;
};



// Update DeleteCommand
class DeleteCommand : public Command {
    DocumentText& buffer;
    std::string deletedText;
    size_t position;

public:
    DeleteCommand(DocumentText& buf, size_t pos, size_t len);

    void execute() override;

    void undo() override;

    [[nodiscard]] size_t getCursorPosition() const override;

    [[nodiscard]] size_t getUndoCursorPosition() const override;
};

// Modify CommandHistory to store the last command
class CommandHistory {
private:
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;
    Command* lastCommand{};
    size_t lastCursorPosition = 0;

public:
    void executeCommand(std::unique_ptr<Command> cmd);

    void undo();
    

    void redo();
    

    [[nodiscard]] size_t getLastCursorPosition() const;
};


#endif // DOCUMENTTEXT_H