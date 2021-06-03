For the poor soul that at some point will try to recompile this:
1) The Internet Archive will hopefully still host something called "VB6 portable". It sounds fake, but actually works, also select the option to install the registry keys for .exe support.
2) Adapt the paths in "VPinMameTest.vbp" to match yours.
3) Make sure that all files in this directory are formatted with windows line endings, otherwise VB6 will be screwed opening them.
4a) Test the compile by pressing the play button and select a game in the dropdown.
4b) File -> Make "XXX.exe" (this may fail, but doing it multiple times worked at some point for me).
