# [NickolasDDiazTextEditor](https://github.com/nickolasddiaz/NickolasDiaz-Text-Editor)

A text editor built using C++ and Windows API.

![image](https://github.com/user-attachments/assets/c97cb56a-fb64-4857-b79d-3186279850f8)

## Features

### Multi-Document Interface
- **Tabbed Interface**:
- **Create New Files**: 
- **Open Existing Files**: 

### File Operations
- **Save**:
- **Save As**: 
- **Save All**:

### Editing Capabilities
- **Undo/Redo**
- **Cut, Copy, and Paste**

## Technical Details

- **Language**: C++
- **Build System**: CMake
- **Main Components**:
  - `TextEditor`: Main application class
  - `TabControl`: Manages the tabbed interface
  - `DocumentText`: Handles text storage and manipulation
- **Key Files**:
  - `TextEditor.cpp`: Core editor functionality
  - `TabControl.cpp`: Tab management
  - `DocumentText.cpp`: Text document handling

## Getting Started
Download from the release [https://github.com/nickolasddiaz/NickolasDiaz-Text-Editor/blob/master/nickolasddiazeditor.exe](https://github.com/nickolasddiaz/NickolasDiaz-Text-Editor/releases)

To build and run NickolasDDiazTextEditor:

1. Ensure you have CMake and a C++ compiler installed.
2. Clone the repository.
3. Navigate to the project directory.
4. Run the following commands:
   ```
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```
## Inspired by
https://austinhenley.com/blog/challengingprojects.html

https://www.catch22.net/tuts/neatpad/

   
## Gap Buffer Implementation
The Gap Buffer is a data structure used in text editing to efficiently manage text insertion and deletion. It involves an array with a "gap" between two pieces of text, which allows for efficient editing around that gap

Structure: The gap buffer is implemented in the DocumentText class. It consists of a character array (buffer) with a "gap" in the middle.
Gap Positioning: The gap is defined by two indices: gapStart and gaspEnd. Text before gapStart and after gapEnd represents the actual document content.
Insertion: When text is inserted, it's placed at the start of the gap. The gapStart is then moved forward, effectively shrinking the gap.
Deletion: When text is deleted, the gap expands to cover the deleted region. This is done by adjusting gapStart or gapEnd.
Cursor Movement: When the cursor moves, the gap is relocated to the cursor position using the moveGap function. This involves copying text from one side of the gap to the other.
Efficiency: This approach makes insertions and deletions at or near the cursor position very efficient, typically O(1) operations.
Buffer Expansion: If the gap becomes too small to accommodate new text, the expandBuffer function is called to increase the buffer size.

## Command Pattern for Undo/Redo
The editor implements the Command Pattern to provide undo and redo functionality:

Command Interface: An abstract Command class defines the interface for all commands with execute() and undo() methods.
Concrete Commands:

InsertCommand: Represents text insertion operations.
DeleteCommand: Represents text deletion operations.


Command Execution: When a user types or deletes text, a corresponding command object is created and executed. The command object stores all necessary information to perform and reverse the action.
Command History: The CommandHistory class maintains two stacks:

undoStack: Stores executed commands
redoStack: Stores undone commands


Undo Operation:

Pops the top command from the undoStack
Calls its undo() method
Pushes the command onto the redoStack


Redo Operation:

Pops the top command from the redoStack
Calls its execute() method
Pushes the command back onto the undoStack


Cursor Position Tracking: Each command stores the cursor position after its execution, allowing for accurate cursor placement during undo/redo operations.
