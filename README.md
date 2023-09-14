Quake-WASM
==========

This is a WebAssembly port of the famous 3D first-person shooter Quake.

Playing a Free Version
----------------------

You can play a game based on the Quake engine [here](https://gmh-code.github.io/quake-free/).

This runs entirely in your browser and doesn't require anything from the original game.

Playing the Shareware / Full Version
------------------------------------

If you already have the shareware version of the Quake, or know where to get it, you need to extract the resource file `PAK0.PAK`.  If you have the full version, you can add `PAK1.PAK`.

Providing you have the relevant file(s), you can start the game [here](https://gmh-code.github.io/quake/).

It is also possible to run mods if you supply additional `PAK` files, e.g. `PAK2.PAK`, `PAK3.PAK`, etc.

Obtaining Required Files
------------------------

### PAK0.PAK

It is straightforward to obtain `PAK0.PAK`:

- Download and extract the **Quake 1.06** shareware version (`quake106.zip` -- widely available online).
- Extract `resource.1` -- Modern versions of 7-Zip and WinRAR, and probably other popular archivers, can do this.
- The `PAK0.PAK` file is in the `ID1` folder.

### Shareware Note

The licence for the shareware version of Quake only appears to permit duplication of the archive that was originally obtained from an official source, so it appears as though, understandably, their contents cannot be distributed by themselves.  John Carmack's statement in the release of Quake's source code, the current state of Linux ports, and other findings on the Internet, also suggest the licence hasn't changed.

I am aware that there are various DOOM ports running online (and offline) that embed the IWAD game data to enable an automatic start, but it looks like this is *not* permitted for Quake.

### PAK1.PAK

`PAK1.PAK` file is needed in addition to `PAK0.PAK` if you want features from the complete version.  This file is provided when you buy the full game.

### Checksums

For security, I recommend only using the *SHA256* hash to verify these files' contents.

    Filename: quake106.zip
    File size: 9,094,045 bytes
    MD5: 8CEE4D03EE092909FDB6A4F84F0C1357
    SHA1: F8A1A509B094CCDBED3C54B96F7D9B351C0898F5
    SHA256: EC6C9D34B1AE0252AC0066045B6611A7919C2A0D78A3A66D9387A8F597553239

    Filename: PAK0.PAK
    File size: 18,689,235 bytes
    MD5: 5906E5998FC3D896DDAF5E6A62E03ABB
    SHA1: 36B42DC7B6313FD9CABC0BE8B9E9864840929735
    SHA256: 35A9C55E5E5A284A159AD2A62E0E8DEF23D829561FE2F54EB402DBC0A9A946AF

    Filename: PAK1.PAK
    Filesize: 34,257,856 bytes
    MD5: D76B3E5678F0B64AC74CE5E340E6A685
    SHA1: 6FD0D1643047612F41AFB2001B972D830C792921
    SHA256: 94E355836EC42BC464E4CBE794CFB7B5163C6EFA1BCC575622BB36475BF1CF30

In-Browser Saving
-----------------

If you use the `Save` option or change the settings in-game, *Quake-WASM* will attempt to commit those changes to browser storage.  These changes should persist after a browser reload.

Saving `PAK` files to storage isn't implemented as this can cause a lag when the filesystem is synched.

Performance
-----------

Quake-WASM performs consistently well and will almost always hit 60FPS in a web browser, even on performance-limited systems.

Testing involved running identical `timerefresh` and `timedemo` benchmarks, ensuring versions were configured with the same settings.

### Quake-WASM vs. WebAssembly DosBox

This version performed about 50 - 60 times faster than it did with the WebAssembly version of DosBox, with the optimised x86 assembler code compiled and linked.  DosBox is obviously going to be at a serious disadvantage here since running a general-purpose x86 emulator in a web browser is very inefficient; *Quake-WASM* bypasses that layer and so is closer to native code.

### Quake-WASM vs. Native DosBox

When testing, this version (still running in a browser) was consistently around twice as fast as running Quake in DosBox on an x86 system, even with the x86 assembler included.

### Notes on x86 Assembler

This version does *not* include the x86 assembler code.  Instead, the cross-platform C substitutes are swapped in, and optimised compiler settings are used.  *Quake-WASM* doesn't have the x86 advantage in WebAssembly, however, it *does* have the advantage of being able to run across different CPU architectures without recompilation, such as on ARM CPUs.

Mods and QuakeC Support
-----------------------

Mods written in QuakeC, which worked in the original version of Quake, should also run in WebAssembly.

QuakeC is often referred to as compiled, but, to clarify, it actually compiles to bytecode rather than native machine mode, similar to *CPython*.  This bytecode is interpreted at runtime.  Since this interpreter works in WebAssembly, most Quake 1 mods should be playable within your browser.

Custom maps, models, textures, sounds, textures and animated sprites should also work, especially if they worked on the original DOS or Windows versions.

Registered Version Detection
----------------------------

Like the original game, *Quake-WASM* requires `gfx/pop.lmp` to be present to detect if you're running the full version.  However, this version allows that file to contain any contents, or it can be 0 bytes in size.  You can add this folder and file yourself, if you wish.

Simply forcing the game to run registered by default is not a good idea because it:

- Breaks proper compatibility with the official shareware version, most notably when accessing levels in the *Introduction*.
- Potentially breaks third-party add-ons that use assets from the full version, if only the shareware files are present.

GLQuake and WebGL Hardware-Accelerated Graphics
-----------------------------------------------

It is possible to add OpenGL -> WebGL support to this version, allowing GLQuake to run GPU-accelerated in a web browser.  The game builds and runs, but no objects render and there are JavaScript console warnings regarding missing GL translation support.  This is why only the software-rendering version has been supplied, for now at least.

Emscripten has limited emulation for the old-style OpenGL Fixed Function Pipeline (FFP).  Quake 1 does not use shaders, whereas WebGL and OpenGL ES does.  Building with *Regal* as a compatibility layer also hasn't worked yet.

Networking Support
------------------

You can play in a browser window, but WebSockets support for multiplayer has not yet been added.

It should be possible to connect to a WebSockets proxy to enable online play, but the project will need rebuilding with the appropriate proxy configuration.

Building from Source (on Linux)
-------------------------------

Before you start:

1. As mentioned earlier, don't bundle or host any files unless you have a licence to do so.
2. You will need to add files to the `WinQuake/id1` folder (`PAK` or otherwise).  If you don't do this, the engine will have nothing to run.

Next, download and extract [Emscripten](https://emscripten.org/), then run these commands to start the build:

    cd <EMSDK folder>
    ./emsdk install latest
    ./emsdk activate latest
    source ./emsdk_env.sh
    cd <Quake folder>/WinQuake
    make -f Makefile.emscripten

This will output `index.html`, `index.js`, `index.wasm`, and `index.data`, which can be placed into an empty directory on a web server.

For fastest download time, compress these files with *GZip* or *Brotli* and ensure they are served as-is.

Notes
-----

When building *Quake-WASM*, all filenames included in the `id1` folder should be in lowercase.  The Quake engine is case-sensitive on anything other than Windows.
