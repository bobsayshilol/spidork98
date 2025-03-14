"When DOSBox-X launches, run these to mount the X drive and activate DJGPP"
""
"Z:\>dev.bat"
""
"Z:\>X:"
""
"X:\>cd example"
""
"X:\EXAMPLE>make"
""
"X:\EXAMPLE>example.exe"
""
Read-Host -Prompt "Press the any key to continue" | Out-Null

"Launching DOSBox-X"
cd dosbox-x\mingw-build\mingw
./dosbox-x.exe | Out-Null
