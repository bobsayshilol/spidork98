"Downloading zips"
Invoke-WebRequest "https://www.target-earth.net/wiki/lib/exe/fetch.php?media=blog:pc-9801_gcc-v2.95.2_djgpp-v2.03.zip" -OutFile djgpp.zip
Invoke-WebRequest "https://github.com/joncampbell123/dosbox-x/releases/download/dosbox-x-v2025.02.01/dosbox-x-mingw-win32-lowend9x-20250201150724.zip" -OutFile dosbox-x.zip

"Extracting zips"
Expand-Archive -Path djgpp.zip -DestinationPath x
Expand-Archive -Path dosbox-x.zip -DestinationPath dosbox-x

"Copying dev.bat"
$xpath = Join-Path $PSScriptRoot "x"
(Get-Content dev.bat.in).Replace('YOUR_X_PATH', $xpath) | Set-Content dosbox-x\mingw-build\mingw\drivez\dev.bat

"Development environment ready to go!"
Read-Host -Prompt "Press the enter key to close" | Out-Null
