Write-Host "Building PaCman"
clang pacman.c -o pacman ; if ($?) { Write-Host "Building complete. Running PaCman..." ; conhost.exe ".\pacman.exe" }
