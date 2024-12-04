# Dot Matrix Sheet

This project simulates a dot matrix sheet using SDL2. The dots are connected by springs and can be dragged with the mouse.

## Dependencies

- **SDL2**: Simple DirectMedia Layer 2.0

To install SDL2 using Homebrew, run the following command:

```sh
brew install sdl2
```

## Build and Run

To build the dot_matrix_sheet.c file, use the following command:

```sh
gcc -o dot_matrix_sheet [dot_matrix_sheet.c](http://_vscodecontentref_/0) -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -lSDL2 -lm
```

To run the compiled executable, use the following command:

```sh
./dot_matrix_sheet
```
