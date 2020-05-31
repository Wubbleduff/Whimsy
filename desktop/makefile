
INTERMEDIATE=./intermediate/

RUN_TREE=./run_tree/

EXE=desktop.exe

PDB=desktop.pdb

SOURCE=src/main.cpp src/desktop.cpp src/input.cpp src/particles.cpp src/profiling.cpp lib/imgui/imgui.cpp lib/imgui/imgui_demo.cpp lib/imgui/imgui_draw.cpp lib/imgui/imgui_widgets.cpp lib/imgui/examples/imgui_impl_opengl3.cpp lib/imgui/examples/imgui_impl_win32.cpp

INCLUDE_DIRS=/I"lib/glew-2.1.0/include" /I"lib/imgui" /I"lib/stb"

LIBS=user32.lib gdi32.lib shell32.lib opengl32.lib lib/glew-2.1.0/lib/Release/x64/glew32.lib

DEBUG_COMPILE_FLAGS=/c /Z7 /EHsc /Fo$(INTERMEDIATE)
DEBUG_LINK_FLAGS=/DEBUG:FULL /OUT:$(INTERMEDIATE)$(EXE)

RELEASE_COMPILE_FLAGS=/c /O2 /EHsc /Fo$(INTERMEDIATE)
RELEASE_LINK_FLAGS=/DEBUG:NONE /OUT:$(INTERMEDIATE)$(EXE)

debug: | intermediate run_tree
	cl $(DEBUG_COMPILE_FLAGS) $(INCLUDE_DIRS) $(SOURCE)
	link $(DEBUG_LINK_FLAGS) $(LIBS) $(INTERMEDIATE)*.obj
	cp -f $(INTERMEDIATE)$(EXE) $(RUN_TREE)
	cp -f $(INTERMEDIATE)$(PDB) $(RUN_TREE)
	cp -rf textures/ run_tree

release: | intermediate run_tree
	cl $(RELEASE_COMPILE_FLAGS) $(INCLUDE_DIRS) $(SOURCE)
	link $(RELEASE_LINK_FLAGS) $(LIBS) $(INTERMEDIATE)*.obj
	cp -f $(INTERMEDIATE)$(EXE) $(RUN_TREE)
	#cp -f $(INTERMEDIATE)$(PDB) $(RUN_TREE)
	cp -rf textures/ run_tree

clean: intermediate
	rm -f $(INTERMEDIATE)*.obj
	rm -f $(INTERMEDIATE)*.exe
	rm -f $(INTERMEDIATE)*.pdb
	rm -f $(INTERMEDIATE)*.ilk


# Building directory rules
intermediate:
	mkdir $(INTERMEDIATE)

run_tree:
	mkdir $(RUN_TREE)

