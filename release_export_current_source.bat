@cls
@rem *** check parameter
@if "%1a"=="a" goto error
@echo *** !!! The password of anonymous is blank so just press enter !!! ***
cvs -d:pserver:anonymous@pinmame.cvs.sourceforge.net:/cvsroot/pinmame login
@rem *** most current source
cvs -z3 -d:pserver:anonymous@pinmame.cvs.sourceforge.net:/cvsroot/pinmame export -DNOW -d "%1" pinmame
@rem *** logout
cvs -d:pserver:anonymous@pinmame.cvs.sourceforge.net:/cvsroot/pinmame logout
@goto end

:error
@echo Please state the path to which the source code should be exported.

:end
@pause
