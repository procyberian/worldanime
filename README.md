# World Anime (ASCII Globe)

A terminal-based C project that renders an animated ASCII Earth with color, lighting, and interactive controls.

## Purpose

This project demonstrates how to build a real-time terminal renderer in plain C by combining:

- ASCII frame rendering
- ray/sphere shading math
- geographic texturing (land vs ocean mask)
- keyboard input in raw terminal mode
- simple command-line interface design

It is useful as a learning project for low-level graphics logic, terminal UX, and interactive systems programming without external graphics libraries.

## Features

- Animated globe rotation in terminal
- Colored land, ocean, and atmospheric rim
- Day/night style shading and highlights
- Vertical tilt control
- Runtime speed control
- CLI options for initial spin and speed
- Clean terminal restore on quit (cursor + mode)

## Build

Requirements:

- Linux/macOS terminal with ANSI color support
- GCC
- libm (math library, linked with `-lm`)

Build command:

```bash
gcc -O2 -o world_animation world_animation.c -lm
```

## Run

```bash
./world_animation
```

## CLI Help / Options

The program supports these options:

- `-s SPIN`  Initial horizontal rotation in degrees (default: 0.0)
- `-v SPEED` Rotation speed in degrees/frame (default: 1.5)
- `-h`       Show help and exit
- `--help`   Show help and exit

Examples:

```bash
./world_animation -s 180
./world_animation -v 3.0
./world_animation -v -2.0
./world_animation -s 90 -v -2.0
./world_animation -s 35
```

Notes:

- Negative speed values rotate in reverse.
- Starting with `-s 35` approximately centers the view around Turkey longitude.

## Interactive Controls

While running:

- Left / Right arrows: decrease / increase rotation speed by 0.5 degrees per frame
- Up / Down arrows: tilt globe north / south
- `+` / `-`: multiply / divide speed by 1.3x
- `q`: quit
- Ctrl+C: quit

## Data Source

Current land/ocean mask in source is NASA-derived.

- Source raster: https://eoimages.gsfc.nasa.gov/images/imagerecords/57000/57730/land_ocean_ice_2048.png
- Attribution: NASA Visible Earth

## File Overview

- `world_animation.c`: main program source code
- `ChangeLog`: development change history
- `LICENSE`: project license text

## License

ASCII Globe Animation data from NASA in C Language

Copyright (C) 2026  PSD Authors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
