param([string]$SodiumRoot = $env:SODIUM_ROOT, [string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$build = Join-Path $repo 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$flags = @('-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic', '-O2', "-I$(Join-Path $repo 'include')")
$libs = @()
if ($SodiumRoot) {
    $flags += @('-DSODIUM_STATIC', "-I$(Join-Path $SodiumRoot 'include')")
    $libs += @((Join-Path $SodiumRoot 'lib\libsodium.a'), '-lws2_32', '-ladvapi32')
} else {
    $libs += '-lsodium'
}
$core = @(Get-ChildItem (Join-Path $repo 'src') -Filter '*.cpp' | Where-Object Name -ne 'main.cpp' | ForEach-Object FullName)
& $Compiler @flags @core (Join-Path $repo 'src\main.cpp') @libs -o (Join-Path $build 'quiz.exe')
if ($LASTEXITCODE -ne 0) { throw 'Application compilation failed' }
$tests = @(Get-ChildItem (Join-Path $repo 'tests') -Filter '*.cpp' | ForEach-Object FullName)
& $Compiler @flags @core @tests @libs -o (Join-Path $build 'quiz_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
Write-Output 'Built build/quiz.exe and build/quiz_tests.exe'
