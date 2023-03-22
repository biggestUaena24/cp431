# CP431 - Parallel Programming

## Setup for Julia Set C Code

### Install dependency

`sudo apt install libpng-dev`

### Compile and run

`gcc juliaset.c -lpng`

## FFMPEG Frames to Gif

`ffmpeg -i animation/frame_%04d.png -vf "fps=10,scale=800:-1:flags=lanczos" -c:v gif output.gif`
