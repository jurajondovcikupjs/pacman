$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    $fontInput = Read-Host "Enter console font size (default 18)"
    $fontSize = 0
    if ([string]::IsNullOrWhiteSpace($fontInput)) {
        $fontSize = 18
    } elseif (-not [int]::TryParse($fontInput, [ref]$fontSize) -or $fontSize -lt 8 -or $fontSize -gt 72) {
        throw "Font size must be a number from 8 to 72."
    }

    Write-Host "Building PaCman with font size $fontSize"
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

    Write-Host "Building complete. Running PaCman..."
    conhost.exe ".\pacman.exe"
    if ($LASTEXITCODE -eq 0) { Write-Host "Thank you for playing!" }
} finally {
    Pop-Location
}
