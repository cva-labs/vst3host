$ErrorActionPreference = 'Stop'

$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$generator = 'Visual Studio 17 2022'

if ($cmakeCommand) {
    $cmake = $cmakeCommand.Source
} elseif (Test-Path 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe') {
    $cmake = 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    $generator = 'Visual Studio 18 2026'
} elseif (Test-Path 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe') {
    $cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
} else {
    throw 'CMake was not found. Install CMake 3.22+ or the Visual Studio CMake component.'
}

& $cmake -S $PSScriptRoot -B "$PSScriptRoot/build" -G $generator -A x64
& $cmake --build "$PSScriptRoot/build" --config Release

$exe = Join-Path $PSScriptRoot 'build/VST3PlayerHost_artefacts/Release/VST3 Player Host.exe'
if (Test-Path -LiteralPath $exe) {
    Write-Host "Build complete: $exe"
} else {
    Write-Host 'Build completed, but the executable was not found at the expected path.'
}
