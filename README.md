# PaCman

## Run the game
Standart command line window is enough, but not good enough. That's why `conhost.exe` is used.

1. Make sure `pacman.exe` is in this folder.
2. Run:
```powershell
.\run.ps1
```
to launch the game using Windows' configured console host.

## Build and run

The build script prompts for a console font size from 8 to 32. Press Enter to use the default size of 18 (which is the console's default size, 1080p screen).
To compile the game and then launch it in `conhost.exe`, run:

```powershell
.\build.ps1
```

## Controls

Use W/A/S/D or the arrow keys to move. Press P to pause or resume, F to save, and Q to quit.
Use F to save the game. Use L to load a saved game.

## Powerups

Powerups are items that give the player special abilities.
Currently, there are five types of powerups:
- Freeze, which freezes the ghosts for a short period of time,
- Speed, which increases the player's speed,
- Repel, which repels the ghosts away from the player,
- Shield, which protects from being hit by ghosts once,
- Double score, which doubles the player's score.
