"Downloading zips"
#Invoke-WebRequest "https://www.target-earth.net/wiki/lib/exe/fetch.php?media=blog:pc-9801_gcc-v2.95.2_djgpp-v2.03.zip" -OutFile djgpp.zip
#Invoke-WebRequest "https://github.com/joncampbell123/dosbox-x/releases/download/dosbox-x-v2025.05.03/dosbox-x-mingw-win32-lowend9x-20250503164337.zip" -OutFile dosbox-x.zip

"Extracting zips"
# Emulator
Expand-Archive -Path dosbox-x.zip -DestinationPath dosbox-x
# DJGPP
Expand-Archive -Path djgpp.zip -DestinationPath x
# Turbo C++
#Expand-Archive -Path TC4.zip -DestinationPath y
#Expand-Archive -Path TASM.zip -DestinationPath y
#Expand-Archive -Path TD.zip -DestinationPath y
#Expand-Archive -Path TPROF.zip -DestinationPath y

"Copying dev.bat"
(Get-Content dev.bat.in).Replace('DRIVES_PATH', $PSScriptRoot) | Set-Content dosbox-x\mingw-build\mingw\drivez\dev.bat

"Development environment ready to go!"
Read-Host -Prompt "Press the enter key to close" | Out-Null
