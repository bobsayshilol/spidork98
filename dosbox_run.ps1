"Launching DOSBox-X, simulating a PC-9821AS2 (or close enough)"
cd dosbox-x\mingw-build\mingw
./dosbox-x.exe `
  -set machine=pc9821 `
  -set cputype=486_prefetch `
  -set fpu=false `
  -set cycles=11000 `
  -set output=surface `
  -set sensitivity=1,1 `
  -set memsize=3 `
  -set xms=true `
  -c "dev.bat" `
  | Out-Null
