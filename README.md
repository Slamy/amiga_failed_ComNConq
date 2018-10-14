# amiga_failed_ComNConq
This is an abandoned project of a failed attempt to create a Command &amp; Conquer clone on the Amiga.
I've decided to upload it here as people might use it as a starting point for learning working with the bebbo toolchain.

The code quality of this project is questionable. The performance of the BOB engine is catastrophic.
But the 8-direction scroller is working very good I guess. I've waste some hours into perfecting it.
Also there is a A* implementation available which works awful on an 68000.

Dependencies:

Install a bebbo toolchain and add the binaries to your PATH.
https://github.com/bebbo/amigaos-cross-toolchain

You can check everything is set correctly if you can use vasm and m68k-amigaos-gcc.

How to build:
```bash
mkdir ../amiga_failed_ComNConq_build
cd ../amiga_failed_ComNConq_build
cmake ../amiga_failed_ComNConq
make
```

The resulting binary is game inside src.

How to play:

Press and hold the right mouse button to activate scroll.
Select a "rotating crystal harvester" with the left mouse and press H on the keyboard. It will start harvesting.
The "tanks" can be controlled as well.


Disclaimer:
This software is provided AS IS. If it damages your Amiga our attached hardware in any way it's your fault for executing it.
The music used in this game is from STune and is called "Aliens on Earth". Copyright infrigement is not intented. The rights of this song belongs to the original author of that game.
