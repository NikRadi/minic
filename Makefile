CCFLAGS = /nologo /cgthreads1 /Od /W4 /ZI /sdl
OBJDIR = obj


all: init
	@cl $(CCFLAGS) src/*.c /Fo./$(OBJDIR)/ /Fe:clittle.exe


init:
	@IF NOT EXIST $(OBJDIR) MKDIR $(OBJDIR)


test: all
	@clittle.exe
	@nasm -f win64 -o TestMain.obj TestMain.asm
	@link TestMain.obj /defaultlib:msvcrt.lib /defaultlib:legacy_stdio_definitions.lib /defaultlib:Kernel32.lib /subsystem:console /out:TestMain.exe
	@TestMain.exe

clean:
	@IF EXIST *.asm DEL *.asm
	@IF EXIST *.exe DEL *.exe
	@IF EXIST *.idb DEL *.idb
	@IF EXIST *.ilk DEL *.ilk
	@IF EXIST *.pdb DEL *.pdb
	@IF EXIST *.obj DEL *.obj
	@IF EXIST $(OBJDIR) RMDIR /Q /S $(OBJDIR)
