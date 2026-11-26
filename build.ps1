$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    Write-Host "Building PaCman"
    llvm-rc /fo pacman.res pacman.rc
    if ($LASTEXITCODE -ne 0) { throw "Icon resource compilation failed." }

    $target = clang -dumpmachine
    if ($LASTEXITCODE -ne 0) { throw "Could not determine Clang's target architecture." }
    $machine = switch -Regex ($target) {
        '^x86_64-' { 'X64'; break }
        '^i[3-6]86-' { 'X86'; break }
        '^aarch64-' { 'ARM64'; break }
        default { throw "Unsupported Clang target: $target" }
    }

    llvm-cvtres /machine:$machine /out:pacman.res.o pacman.res
    if ($LASTEXITCODE -ne 0) { throw "Icon resource conversion failed." }

    clang pacman.c pacman.res.o -o pacman.exe -luser32
    if ($LASTEXITCODE -ne 0) { throw "PaCman compilation failed." }

    Write-Host "Build complete. Launching PaCman in a CMD window..."
    $exePath = Join-Path $PSScriptRoot 'pacman.exe'
    $cmdArguments = '/c mode con: cols=45 lines=37 && ""{0}""' -f $exePath
    $process = Start-Process -FilePath $env:ComSpec -ArgumentList $cmdArguments `
        -WorkingDirectory $PSScriptRoot -Wait -PassThru
    if ($process.ExitCode -eq 0) { Write-Host "Thank you for playing!" }
} finally {
    Pop-Location
}
