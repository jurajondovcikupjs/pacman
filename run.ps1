Write-Host "Launching PaCman in a conhost window..."
$exePath = Join-Path $PSScriptRoot 'pacman.exe'
$conhostPath = Join-Path $env:WINDIR 'System32\conhost.exe'
& $conhostPath $exePath
if ($LASTEXITCODE -eq 0) { Write-Host "Thank you for playing!" }
