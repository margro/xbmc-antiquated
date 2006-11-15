@ECHO OFF
cls

rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem ---------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem ONLY needed if you have a very old bios, SET the path for xbepatch. Not needed otherwise.
rem If Winrar isn't installed under standard programs, SET the path for WinRAR's (freeware) rar.exe
rem and finally set the options for the final rar.
rem ---------------------------------------------
rem Remove 'rem' from 'web / python' below to copy these to the BUILD directory.
rem ---------------------------------------------
TITLE XBMC Build Prepare Script
ECHO Wait while preparing the build.
ECHO ------------------------------
rem	CONFIG START
	IF "%VS71COMNTOOLS%"=="" (
	  set NET="%ProgramFiles%\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.com"
	) ELSE (
	  set NET="%VS71COMNTOOLS%\..\IDE\devenv.com"
	)
	IF NOT EXIST %NET% (
	  set DIETEXT=Visual Studio .NET 2003 was not found.
	  goto DIE
	) 
	set OPTS=xbmc.sln /build release
	set CLEAN=xbmc.sln /clean release
	set XBE=xbepatch.exe
	set RAR="%ProgramFiles%\Winrar\rar.exe"
	set RAROPS=a -r -idp -inul -m5 XBMC.rar BUILD
rem	CONFIG END
rem ---------------------------------------------
ECHO Compiling Solution...
%NET% %CLEAN%
del release\xbmc.map
%NET% %OPTS%
IF NOT EXIST release\default.xbe (
	set DIETEXT=Default.xbe failed to build!  See .\Release\BuildLog.htm for details.
	goto DIE
) 
ECHO Done!
ECHO ------------------------------
ECHO Copying files...
%XBE% release\default.xbe
rmdir BUILD /S /Q
md BUILD

Echo .svn>exclude.txt
Echo Thumbs.db>>exclude.txt
Echo Desktop.ini>>exclude.txt
Echo dsstdfx.bin>>exclude.txt

copy release\default.xbe BUILD
xcopy UserData BUILD\UserData /E /Q /I /Y /EXCLUDE:exclude.txt
rem copy *.xml BUILD\
copy *.txt BUILD\

cd "skin\Project Mayhem III"
CALL build.bat
cd ..\..
xcopy "skin\Project Mayhem III\BUILD\Project Mayhem III" "BUILD\skin\Project Mayhem III" /E /Q /I /Y /EXCLUDE:exclude.txt

xcopy credits BUILD\credits /Q /I /Y /EXCLUDE:exclude.txt
xcopy language BUILD\language /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy screensavers BUILD\screensavers /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy visualisations BUILD\visualisations /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy system BUILD\system /E /Q /I /Y /EXCLUDE:exclude.txt
rem %rar% x web\Project_Mayhem_webserver*.rar build\web\
xcopy media BUILD\media /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy sounds BUILD\sounds /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt

ECHO ------------------------------
IF NOT EXIST %RAR% (
	ECHO WinRAR not installed!  Skipping .rar compression...
) ELSE (
	ECHO Compressing build to XBMC.rar file...
	%RAR% %RAROPS%
)

ECHO ------------------------------
ECHO Build Succeeded! 
GOTO VIEWLOG

:DIE
ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
set DIETEXT=ERROR: %DIETEXT%
echo %DIETEXT%

:VIEWLOG
set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
if /I %XBMC_BUILD_ANSWER% NEQ y goto END
start /D"%~dp0Release" BuildLog.htm"
goto END

:END
set XBMC_BUILD_ANSWER=
ECHO Press any key to exit...
pause > NUL