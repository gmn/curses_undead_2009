@echo off

set CURNAME=cursesundead
set RLSNAME=zombie

if exist %RLSNAME%.zip del %RLSNAME%.zip
if exist %RLSNAME%.7z del %RLSNAME%.7z
if exist %RLSNAME% rd /s /q %RLSNAME%
if exist zpak\maps\*.bak del zpak\maps\*.bak
mkdir %RLSNAME%
if not exist %CURNAME%.exe goto error
copy %CURNAME%.exe %RLSNAME%\swurv.exe
xcopy /C /R /Y /E /I zpak %RLSNAME%\zpak
if exist %RLSNAME%\zpak\gfx\hoody.ico del %RLSNAME%\zpak\gfx\hoody.ico
if exist %RLSNAME%\zpak\gfx\robot.ico del %RLSNAME%\zpak\gfx\robot.ico
if exist %RLSNAME%\zpak\player.cfg del %RLSNAME%\zpak\player.cfg
if exist %RLSNAME%\zpak\.hg rd /s /q %RLSNAME%\zpak\.hg
if exist %RLSNAME%\zpak\gfx\.hg rd /s /q %RLSNAME%\zpak\gfx\.hg
if exist %RLSNAME%\zpak\maps\.hg rd /s /q %RLSNAME%\zpak\maps\.hg
if exist %RLSNAME%\zpak\music\.hg rd /s /q %RLSNAME%\zpak\music\.hg
if exist %RLSNAME%\zpak\sound\.hg rd /s /q %RLSNAME%\zpak\sound\.hg
if exist %RLSNAME%\zpak\scripts\.hg rd /s /q %RLSNAME%\zpak\scripts\.hg
REM if exist CREDITS copy CREDITS %RLSNAME%
if exist README.txt copy README.txt %RLSNAME%
zip -r %RLSNAME%.zip %RLSNAME%
REM 7za a %RLSNAME%.7z %RLSNAME%
REM rd /s /q %RLSNAME%
echo.   
echo ************************************
echo.  
echo     RELEASE CREATED SUCCESSFULLY
echo.  
echo  created:
if exist %RLSNAME% echo    %RLSNAME% 
if exist %RLSNAME%.zip echo    %RLSNAME%.zip 
if exist %RLSNAME%.7z echo    %RLSNAME%.7z 
echo.  
echo ************************************
echo.  
goto goodstuff

:error
rd /s /q %RLSNAME%
echo.
echo     ***********************************************************
echo.
echo                        EXECUTABLE MISSING! 
echo.
echo           You probably forgot to build the project first!!
echo.
echo     ***********************************************************
echo.

:goodstuff
