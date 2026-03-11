param(
  [string]$BuildDir = "build/release"
)

$ErrorActionPreference = "Stop"

cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=Release
cmake --build $BuildDir --target ppe ppe_gui

$installDir = "$BuildDir/install"
cmake --install $BuildDir --prefix $installDir

# Bundle Qt runtime if available
$windeploy = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($windeploy) {
  & $windeploy.Path "$installDir/bin/ppe_gui.exe"
} else {
  Write-Host "windeployqt not found. Install Qt6 and ensure it is in PATH."
}

cpack --config "$BuildDir/CPackConfig.cmake"
