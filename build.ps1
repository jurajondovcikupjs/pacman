$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    $fontInput = Read-Host "Enter console font size (8-32, default 18)"
    $fontSize = 18
    if (-not [string]::IsNullOrWhiteSpace($fontInput)) {
        $parsedFontSize = 0
        if (-not [int]::TryParse($fontInput, [ref]$parsedFontSize) -or
            $parsedFontSize -lt 8 -or $parsedFontSize -gt 32) {
            throw "Font size must be a whole number from 8 to 32."
        }
        $fontSize = $parsedFontSize
    }

    Write-Host "Building PaCman with console font size $fontSize"
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

    clang "-DCONSOLE_FONT_HEIGHT=$fontSize" pacman.c pacman.res.o -o pacman.exe -luser32
    if ($LASTEXITCODE -ne 0) { throw "PaCman compilation failed." }

    Write-Host "Build complete. Launching PaCman in a conhost window..."
    $exePath = Join-Path $PSScriptRoot 'pacman.exe'
    $conhostPath = Join-Path $env:WINDIR 'System32\conhost.exe'
    & $conhostPath $exePath
    if ($LASTEXITCODE -eq 0) { Write-Host "Thank you for playing!" }
} finally {
    Pop-Location
}
