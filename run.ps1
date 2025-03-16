"If DOSBox-X launches to a black screen, change Video -> Output to Surface."
Read-Host -Prompt "Press the any key to continue" | Out-Null

"Launching DOSBox-X"
cd dosbox-x\mingw-build\mingw
./dosbox-x.exe -machine pc9821 -c "dev.bat" -output surface | Out-Null
