@ECHO OFF
SET BinDir=bin
SET ObjDir=obj

IF NOT DEFINED DevEnvDir (
    CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall" x64
)

IF "%1" == "" (
    ECHO Building...
    IF NOT EXIST %BinDir% MKDIR %BinDir%
    IF NOT EXIST %ObjDir% MKDIR %ObjDir%
    SET Flags=/NOLOGO /cgthreads1 /Od /W4 /ZI /sdl
    CL %Flags% src\*.c /Fo./%ObjDir%/ /Fe:minic.exe
    IF EXIST *.asm MOVE *.asm %BinDir% >NUL
    IF EXIST *.exe MOVE *.exe %BinDir% >NUL
    IF EXIST *.idb MOVE *.idb %BinDir% >NUL
    IF EXIST *.ilk MOVE *.ilk %BinDir% >NUL
    IF EXIST *.pdb MOVE *.pdb %BinDir% >NUL
)

IF "%1" == "test" (
    ECHO Testing...
    SET Libs=/DEFAULTLIB:msvcrt.lib /DEFAULTLIB:legacy_stdio_definitions.lib /DEFAULTLIB:Kernel32.lib
    FOR %%f in (tests/Test*.c) DO (
        ECHO %%~nf.c
        bin\minic.exe tests\%%f
        CD tests
        nasm -f win64 -o %%~nf.obj %%~nf.asm
        LINK /NOLOGO %Libs% /SUBSYSTEM:console %%~nf.obj /OUT:%%~nf.exe
        %%~nf.exe
        DEL %%~nf.asm %%~nf.obj %%~nf.exe
        CD ..
    )
)

IF "%1" == "clean" (
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