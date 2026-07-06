$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
if (-not (Test-Path build/stockflow_tests.exe)) { & ./build.ps1 }
& ./build/stockflow_tests.exe
if ($LASTEXITCODE -ne 0) { throw "Tests failed." }
