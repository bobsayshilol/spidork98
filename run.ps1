"If DOSBox-X launches to a black screen, change Video -> Output to Surface."
""
"Don't forget to change CPU -> Emulate CPU speed -> 486DX 33MHz"
Read-Host -Prompt "Press the enter key to continue" | Out-Null

"Launching DOSBox-X"
cd dosbox-x\mingw-build\mingw
./dosbox-x.exe -machine pc9821 -c "dev.bat" -output surface | Out-Null
