@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64
IF ERRORLEVEL 1 (
  ECHO ERROR! make-addons.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)
CALL make-addons.bat ^game\.libretro$
CALL make-addons.bat ^game\.libretro\.2048$
CALL make-addons.bat ^game\.libretro\.mrboom$
CALL make-addons.bat ^game\.shader\.presets$
CALL make-addons.bat ^peripheral\.joystick$
CALL make-addons.bat ^vfs\.libarchive$
CALL make-addons.bat ^vfs\.rar$
POPD
