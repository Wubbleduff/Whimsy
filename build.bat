@echo off

SETLOCAL

SET my_flags=/EHsc /O2 /Z7 /Zl /Fd /Fewhimsy.exe

SET my_include=/I "..\Libraries\imgui" /I "..\Libraries\glfw\Include" /I "..\Libraries\glew\Include"

REM SET my_source=..\Source\*.cpp ..\Libraries\imgui\*.cpp ..\Libraries\imgui\examples\imgui_impl_glfw.cpp ..\Libraries\imgui\examples\imgui_impl_opengl3.cpp

SET my_source=..\Source\Unit.cpp

SET my_lib=user32.lib opengl32.lib gdi32.lib shell32.lib ..\Libraries\glew\x64\glew32.lib ..\Libraries\glfw\x64\glfw3.lib





pushd .

cd Build
cl %my_flags% %my_include% %my_source% %my_lib% > build_log.txt
color_output.exe build_log.txt

popd

copy Build\whimsy.exe .

