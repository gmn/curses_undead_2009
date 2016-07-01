@rem Script to build Lua under "Visual Studio .NET Command Prompt".
@rem Do not run from this directory; run it from the toplevel: etc\luavs.bat .
@rem It creates lua51.dll, lua51.lib, lua.exe, and luac.exe in src.
@rem (contributed by David Manura and Mike Pall)

@setlocal
@set MYCOMPILE=cl /nologo /MD /O2 /W3 /TC /c /D_CRT_SECURE_NO_DEPRECATE
@set LIBCOMPILE= cl /nologo /FD /MT /O2 /W3 /TC /c /C_CRT_SECURE_NO_DEPRECATE
@set MYLINK=link /nologo
@set MYMT=mt /nologo
@set MYLIB=lib 


@cd lua

@echo Generating lua51.lib lua51.dll
%MYCOMPILE% /DLUA_BUILD_AS_DLL l*.c
@del lua.obj luac.obj

@rem for DLL
%MYLINK% /DLL /out:lua51.dll l*.obj

@rem for LIB
%LIBCOMPILE% /DLUA_BUILD_AS_DLL l*.c
@del lua.obj luac.obj
%MYLIB% /out:..\lua.lib l*.obj

@if exist lua51.dll.manifest^
    %MYMT% -manifest lua51.dll.manifest -outputresource:lua51.dll;2

@if not exist lua51.dll goto no_dll_error
@if not exist lua51.lib goto no_lib_error

@rem @move lua51.lib ..
@rem @move lua51.dll ..

@echo Generating lua.exe
%MYCOMPILE% /DLUA_BUILD_AS_DLL lua.c
%MYLINK% /out:lua.exe lua.obj lua51.lib
if exist lua.exe.manifest^
    %MYMT% -manifest lua.exe.manifest -outputresource:lua.exe

@rem @echo generating luac.exe
@REM %MYCOMPILE% l*.c print.c
@REM del lua.obj linit.obj lbaselib.obj ldblib.obj liolib.obj lmathlib.obj^
@REM     loslib.obj ltablib.obj lstrlib.obj loadlib.obj
@REM %MYLINK% /out:luac.exe *.obj
@REM if exist luac.exe.manifest^
@REM   %MYMT% -manifest luac.exe.manifest -outputresource:luac.exe

@del *.obj *.manifest *.exp
@cd ..

@goto out_of_here

:no_dll_error
@echo.
@echo ERROR: lua DLL failed to build
@echo.
@goto no_msg

:no_lib_error
@echo.
@echo ERROR: lua LIB failed to build
@echo.
@goto no_msg

:out_of_here
@echo.
@echo Lua built successfully
@echo.

:no_msg
