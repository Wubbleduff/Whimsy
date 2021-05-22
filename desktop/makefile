
INTERMEDIATE=./intermediate/

RUN_TREE=./run_tree/

EXE=desktop.exe

PDB=desktop.pdb

SOURCE=src/main.cpp src/desktop.cpp src/input.cpp src/stars.cpp src/profiling.cpp src/common_graphics.cpp src/shader.cpp lib/imgui/imgui.cpp lib/imgui/imgui_demo.cpp lib/imgui/imgui_draw.cpp lib/imgui/imgui_widgets.cpp lib/imgui/examples/imgui_impl_opengl3.cpp lib/imgui/examples/imgui_impl_win32.cpp
#SOURCE=src/whole_ass_source.cpp lib/imgui/imgui.cpp lib/imgui/imgui_draw.cpp lib/imgui/examples/imgui_impl_opengl3.cpp lib/imgui/examples/imgui_impl_win32.cpp

INCLUDE_DIRS=/I"lib/glew-2.1.0/include" /I"lib/imgui" /I"lib/stb" /I"lib/easy_profiler-v2.1.0-msvc15-win64/include"

LIBS=user32.lib gdi32.lib shell32.lib opengl32.lib lib/glew-2.1.0/lib/Release/x64/glew32.lib lib/easy_profiler-v2.1.0-msvc15-win64/lib/easy_profiler.lib

# DLLs
DLL_GLEW=lib/glew-2.1.0/bin/Release/x64/glew32.dll
DLL_EASY_PROFILER=lib/easy_profiler-v2.1.0-msvc15-win64\bin\easy_profiler.dll

DEBUG_MACROS=#/DBUILD_WITH_EASY_PROFILER
RELEASE_MACROS=#/DBUILD_WITH_EASY_PROFILER

DEBUG_COMPILE_FLAGS=/c /Z7 /EHsc /Fo$(INTERMEDIATE) $(DEBUG_MACROS)
DEBUG_LINK_FLAGS=/DEBUG:FULL /OUT:$(INTERMEDIATE)$(EXE)

RELEASE_COMPILE_FLAGS=/c /O2 /EHsc /Fo$(INTERMEDIATE) $(RELEASE_MACROS)
RELEASE_LINK_FLAGS=/DEBUG:NONE /OUT:$(INTERMEDIATE)$(EXE)

debug: | intermediate run_tree
	cl $(DEBUG_COMPILE_FLAGS) $(INCLUDE_DIRS) $(SOURCE)
	link $(DEBUG_LINK_FLAGS) $(LIBS) $(INTERMEDIATE)*.obj
	cp -f $(INTERMEDIATE)$(EXE) $(RUN_TREE)
	cp -f $(INTERMEDIATE)$(PDB) $(RUN_TREE)
	cp -f $(DLL_GLEW) $(RUN_TREE)
	cp -f $(DLL_EASY_PROFILER) $(RUN_TREE)
	cp -rf textures/ run_tree
	cp -rf shaders/ run_tree

release: | intermediate run_tree
	cl $(RELEASE_COMPILE_FLAGS) $(INCLUDE_DIRS) $(SOURCE)
	link $(RELEASE_LINK_FLAGS) $(LIBS) $(INTERMEDIATE)*.obj
	cp -f $(INTERMEDIATE)$(EXE) $(RUN_TREE)
	cp -f $(DLL_GLEW) $(RUN_TREE)
	cp -f $(DLL_EASY_PROFILER) $(RUN_TREE)
	#cp -f $(INTERMEDIATE)$(PDB) $(RUN_TREE)
	cp -rf textures/ run_tree
	cp -rf shaders/ run_tree

clean: | intermediate
	rm -f $(INTERMEDIATE)*.obj
	rm -f $(INTERMEDIATE)*.exe
	rm -f $(INTERMEDIATE)*.pdb
	rm -f $(INTERMEDIATE)*.ilk


# Building directory rules
intermediate:
	mkdir $(INTERMEDIATE)

run_tree:
	mkdir $(RUN_TREE)

