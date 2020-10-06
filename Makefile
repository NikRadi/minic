CCFLAGS = /nologo /cgthreads1 /Od /W4 /ZI /sdl
OBJDIR = obj


all: init
	@cl $(CCFLAGS) src/*.c /Fo./$(OBJDIR)/ /Fe:clittle.exe


init:
	@IF NOT EXIST $(OBJDIR) MKDIR $(OBJDIR)


clean:
	@IF EXIST *.exe DEL *.exe
	@IF EXIST *.idb DEL *.idb
	@IF EXIST *.ilk DEL *.ilk
	@IF EXIST *.pdb DEL *.pdb
	@IF EXIST $(OBJDIR) RMDIR /Q /S $(OBJDIR)
