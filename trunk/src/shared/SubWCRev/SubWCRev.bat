@REM this batch file has to be run from the directory the vproj is in. all the paths are relative to that.
@ECHO OFF
SETLOCAL enabledelayedexpansion
..\shared\SubWCRev\SubWCRev.exe .. "..\shared\SubWCRev\svnrev_template.h" "..\shared\svnrev_temp.h"
IF EXIST "..\shared\svnrev_temp.h" (
	set text1=
	for /f "delims=" %%i in (..\shared\svnrev_temp.h) do (
		set text1=!text1!%%i
	)
	set text2=
	IF EXIST "..\shared\svnrev.h" (
		for /f "delims=" %%i in (..\shared\svnrev.h) do (
			set text2=!text2!%%i
		)
	)
	if !text1!==!text2! (
		echo decided not to modify svnrev.h
		DEL "..\shared\svnrev_temp.h"
	) else (
		IF EXIST "..\shared\svnrev.h" (
			echo updated svnrev.h
		) else (
			echo generated svnrev.h
		)
		MOVE /Y "..\shared\svnrev_temp.h" "..\shared\svnrev.h"
	)
)
ENDLOCAL
exit 0