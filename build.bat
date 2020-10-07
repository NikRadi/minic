@ECHO OFF
SET BinDir=bin
SET ObjDir=obj

IF NOT DEFINED DevEnvDir (
    CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall" x64
)

IF "%1"=="" (
    ECHO Building...
    IF NOT EXIST %BinDir% MKDIR %BinDir%
    IF NOT EXIST %ObjDir% MKDIR %ObjDir%
    SET Flags=/nologo /cgthreads1 /Od /W4 /ZI /sdl
    cl %Flags% src\*.c /Fo./%ObjDir%/ /Fe:minic.exe
    IF EXIST *.asm MOVE *.asm %BinDir% >NUL
    IF EXIST *.exe MOVE *.exe %BinDir% >NUL
    IF EXIST *.idb MOVE *.idb %BinDir% >NUL
    IF EXIST *.ilk MOVE *.ilk %BinDir% >NUL
    IF EXIST *.pdb MOVE *.pdb %BinDir% >NUL
)

IF "%1"=="test" (
    ECHO Testing...
    CALL %BinDir%\minic.exe
	nasm -f win64 -o TestMain.obj TestMain.asm
	link /nologo TestMain.obj /defaultlib:msvcrt.lib /defaultlib:legacy_stdio_definitions.lib /defaultlib:Kernel32.lib /subsystem:console /out:TestMain.exe
	ECHO TestMain.exe
	CALL TestMain.exe
)

IF "%1"=="clean" (
    ECHO Cleaning...
	IF EXIST *.asm DEL *.asm
	IF EXIST *.exe DEL *.exe
	IF EXIST *.idb DEL *.idb
	IF EXIST *.ilk DEL *.ilk
	IF EXIST *.pdb DEL *.pdb
	IF EXIST *.obj DEL *.obj
    IF EXIST %BinDir% RMDIR /Q /S %BinDir%
    IF EXIST %ObjDir% RMDIR /Q /S %ObjDir%
)