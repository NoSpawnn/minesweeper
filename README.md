# Minesweeper TUI written in C
- A terminal based minesweeper clone in C
- Untested on Windows, do let me know if you try it, but I don't *think* it will work
- **NOTE**: If you exit via Ctrl+C, your cursor will be invisible. Re-run the game and quit with `q` instead.

## Usage
1. Clone the repo
2. Build and run
```console
$ gcc ./minesweeper.c -o minesweeper
$ ./minesweeper

# You can pass the number of rows and/or columns
$ ./minesweeper -r <rows> -c <cols>
```
3. Add it to your path if you want... maybe at `~/.local/bin` or something
```console
$ cp minesweeper ~/.local/bin
```

### Controls
| Key              | Action        |
| ---------------- | ------------- |
| <kbd>W</kbd>     | Move Up       |
| <kbd>A</kbd>     | Move Left     |
| <kbd>S</kbd>     | Move Down     |
| <kbd>D</kbd>     | Move Right    |
| <kbd>F</kbd>     | Flag a cell   |
| <kbd>space</kbd> | Reveal a cell |

## TODO
- [x] Board size as CLI arg
- [x] Fair start (only randomize after first reveal)
- [ ] Open all adjacent empty cells when appropriate