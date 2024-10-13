# CMakeList.txt : CMake project for nickolasddiazeditor, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0.26100.0)
set(WindowsSDK_LibDir "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0")
link_directories(${WindowsSDK_LibDir}/um/x64)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("nickolasddiazeditor")

# Add source to this project's executable.
add_executable (nickolasddiazeditor WIN32 "nickolasddiaztexteditor.cpp" "DocumentText.cpp" "DocumentText.h" "TabControl.cpp" "TabControl.h" "TextEditor.cpp" "TextEditor.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET nickolasddiazeditor PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
