# Setup dev environment

## Easy mode

Just run [setup.ps1](setup.ps1)

## Hard

1. Download [DOSBox-X](https://github.com/joncampbell123/dosbox-x/releases/download/dosbox-x-v2025.02.01/dosbox-x-mingw-win32-lowend9x-20250201150724.zip) (from https://dosbox-x.com/)

1. Download [custom DJGPP build](https://www.target-earth.net/wiki/lib/exe/fetch.php?media=blog:pc-9801_gcc-v2.95.2_djgpp-v2.03.zip) (from https://www.target-earth.net/wiki/doku.php?id=blog:pc98_devtools)

1. Extract DOSBox-X zip into `dl\dosbox-x`

1. Extract DJGPP zip into `x\djgpp`

1. Copy [dev.bat.in](dev.bat.in) to `dosbox\mingw-build\mingw\drivez\dev.bat`

1. Open `dosbox\mingw-build\mingw\drivez\dev.bat` and edit `YOUR_X_PATH` to the full path of the `x` folder


# Using dev environment

1. Run `run.ps1`
