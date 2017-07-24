@cls
@rem *** check parameter
@if "%1a"=="a" goto error
@rem *** most current source
svn export --force https://pinmame.svn.sourceforge.net/svnroot/pinmame/trunk "%1"
@goto end

:error
@echo Please state the path to which the source code should be exported.

:end
@pause
