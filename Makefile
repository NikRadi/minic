# /wd4201 Nameless struct/union
# /wd4996 Use strncpy_s instead of strncpy
CCFLAGS=/nologo /std:c11 /TC /W4 /Zi /wd4201 /wd4996
LINKFLAGS=/nologo /subsystem:console /defaultlib:msvcrt.lib /defaultlib:legacy_stdio_definitions.lib /defaultlib:Kernel32.lib
EXENAME=minic
OBJDIR=obj
BINDIR=bin


all:
	IF NOT EXIST $(OBJDIR) MKDIR $(OBJDIR)
	IF NOT EXIST $(BINDIR) MKDIR $(BINDIR)
	cl $(CCFLAGS) src\*.c /Fo.\$(OBJDIR)\ /Fe$(EXENAME).exe
	IF EXIST *.exe MOVE *.exe $(BINDIR)
	IF EXIST *.idb MOVE *.idb $(BINDIR)
	IF EXIST *.ilk MOVE *.ilk $(BINDIR)
	IF EXIST *.pdb MOVE *.pdb $(BINDIR)

test:
	python tests/run_tests.py

test_file:
	python tests/run_tests.py --file $(FILE)

asm:
	nasm -f win64 tmp.asm -o tmp.obj
	link /nologo /subsystem:console /entry:main tmp.obj
