$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force -Path build | Out-Null

$sources = @(
    "src/main.cpp", "src/Application.cpp", "src/Models.cpp",
    "src/Utils.cpp", "src/Inventory.cpp", "src/Storage.cpp"
)

$vsDevCmd = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022\*\Common7\Tools\VsDevCmd.bat" `
    -ErrorAction SilentlyContinue | Select-Object -First 1

if ($vsDevCmd) {
    $sourceList = $sources -join " "
    $coreSources = "tests/tests.cpp src/Models.cpp src/Utils.cpp src/Inventory.cpp src/Storage.cpp"
    $appCommand = "`"$($vsDevCmd.FullName)`" -arch=x64 -no_logo && cl /nologo /std:c++17 /EHsc /W4 /Iinclude /Fo:build\ $sourceList /Fe:build\stockflow.exe"
    & cmd.exe /d /c $appCommand
    if ($LASTEXITCODE -ne 0) { throw "Application build failed." }
    $testCommand = "`"$($vsDevCmd.FullName)`" -arch=x64 -no_logo && cl /nologo /std:c++17 /EHsc /W4 /Iinclude /Fo:build\ $coreSources /Fe:build\stockflow_tests.exe"
    & cmd.exe /d /c $testCommand
    if ($LASTEXITCODE -ne 0) { throw "Test build failed." }
} else {
    & g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude @sources -o build/stockflow.exe -lstdc++fs
    if ($LASTEXITCODE -ne 0) { throw "Application build failed. GCC 8 or newer is required." }
    & g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude tests/tests.cpp `
        src/Models.cpp src/Utils.cpp src/Inventory.cpp src/Storage.cpp `
        -o build/stockflow_tests.exe -lstdc++fs
    if ($LASTEXITCODE -ne 0) { throw "Test build failed." }
}

Write-Host "Build complete: build/stockflow.exe"
