Quake-WASM
==========

This is a WebAssembly port of the famous 3D first-person shooter Quake.

[Run the Quake engine in your browser here](https://gmh-code.github.io/quake/).

Quake-WASM supports:

- Quake -- software rendering.
- GLQuake -- hardware-accelerated rendering, through WebGL.

Playing a Free Version
----------------------

You can play a free version (with bots) using the above link.

This runs entirely in your browser and doesn't require anything from the original game.

Playing the Shareware / Full Version
------------------------------------

If you already have the shareware version of the Quake, or know where to get it, you need to extract the resource file `PAK0.PAK`.  If you have the full version, you can add `PAK1.PAK`.

It is also possible to run mods if you supply additional `PAK` files, e.g. `PAK2.PAK`, `PAK3.PAK`, etc.

Obtaining Required Files
------------------------

### PAK0.PAK

It is straightforward to obtain `PAK0.PAK`:

- Download and extract the **Quake 1.06** shareware version (`quake106.zip` -- widely available online).
- Extract `resource.1` -- Modern versions of 7-Zip and WinRAR, and probably other popular archivers, can do this.
- The `PAK0.PAK` file is in the `ID1` folder.

### PAK1.PAK

`PAK1.PAK` file is needed in addition to `PAK0.PAK` if you want features from the complete version.  This file is provided when you buy the full game.

### Licence Notes

The licence for the shareware version of Quake only appears to permit duplication of the archive that was originally obtained from an official source, so it appears as though, understandably, the archive's contents cannot be distributed nor embedded separately.  John Carmack's statement in the release of Quake's source code, the current state of Linux ports, and other findings on the Internet, also suggest the licence hasn't changed.

I am aware that there are various DOOM ports running online (and offline) that embed IWAD game data to enable an automatic start of the shareware version, but it looks like this is *not* permitted for Quake.

The full version carries an even more restrictive licence -- so do not be tempted to host the full version on a public server!

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
    File size: 34,257,856 bytes
    MD5: D76B3E5678F0B64AC74CE5E340E6A685
    SHA1: 6FD0D1643047612F41AFB2001B972D830C792921
    SHA256: 94E355836EC42BC464E4CBE794CFB7B5163C6EFA1BCC575622BB36475BF1CF30

In-Browser Saving
-----------------

If you use the `Save` option or change the settings in-game, *Quake-WASM* will attempt to commit those changes to browser storage.  These changes should persist after a browser reload.

Saving `PAK` files to storage isn't implemented as this can cause a lag when the filesystem is synched.

Performance
-----------

Quake-WASM performs consistently well and usually synchronises with the 60FPS frame cap in a web browser, even on performance-limited systems.

Testing involved:

- Disabling the browser frame limit (removes the 60FPS/vsync cap)
- Setting `WASM_BENCHMARK` in the build to `1` (removes the browser's 120Hz calls-per-second cap)
- Ensuring all versions were configured with Quake's default settings
- Running identical `timedemo` benchmarks

### Quake-WASM vs. WebAssembly DosBox

This version performed about 95 - 105 times faster than it did with the WebAssembly version of DosBox, with the optimised x86 assembler code compiled and linked.  DosBox is obviously going to be at a serious disadvantage here since running a general-purpose x86 emulator in a web browser is very inefficient; *Quake-WASM* bypasses that layer and so is closer to native code.

### Quake-WASM vs. Native DosBox

When testing, this version (still running in a browser) was consistently around 3 times as fast as running Quake in DosBox on an x86 system, even with the x86 assembler included.

### Notes on x86 Assembler

This version does *not* include the x86 assembler code.  Instead, the cross-platform C substitutes are swapped in, and optimised compiler settings are used.  *Quake-WASM* doesn't have the x86 advantage in WebAssembly, however, it *does* have the advantage of being able to run across different CPU architectures without recompilation, such as on ARM CPUs.

### GLQuake vs Quake

GLQuake performs 2 to 3.5 times as fast as Quake when running in WebGL.  This is dependent on hardware, scene complexity, and other events being processed.  The game can also handle higher resolutions with greater ease.  In my opinion, this version looks far better.

Mods and QuakeC Support
-----------------------

Mods written in QuakeC, which worked in the original version of Quake, should also run in WebAssembly.

Custom maps, models, textures, sounds, and animated sprites should also work, especially if they worked on the original DOS or Windows versions.

Saving is supported, even when hosting several mods on the same URL.  This works exactly the same way as the desktop version, except user data is kept in a separate location.

QuakeC is often referred to as compiled, but it actually compiles to bytecode rather than native machine mode, similar to *CPython*.  This bytecode is then interpreted at runtime.  This interpreter works in WebAssembly, so most Quake 1 mods should be playable within your browser.

Mission Packs
-------------

Quake-WASM supports:

- Mission Pack 1: Scourge of Armagon
- Mission Pack 2: Dissolution of Eternity

The easiest way to use these is to simply rename the mission pack's PAK file as `PAK2.PAK` and supply it with the other files.  The game should then be started with the appropriate command-line parameter (`-hipnotic` or `-rogue` respectively), otherwise some of the extra features will be missing, such as the extended status area.

If you want to check that the mission pack is working properly, start it up, type `impulse 9` in the console, and switch through the weapons.  Special weapons will show within a red box in the status area.

Command-Line Arguments
----------------------

Like the desktop version of Quake, you can pass arguments to this version at runtime.  By default, the query portion of the URL is used, but the JavaScript code can be modified to use anything else, such as an input text box.

### Example

Let's say you were using the default template, serving the page locally, and you wanted to:

1. Start the engine with the canvas resolution set to 1152x864, and
2. Run the console command that opens the 'Load Game' menu.

To do this you would normally use something like `-winsize 1152 864 +menu_load`.

In *Quake-WASM*, you can append a single `?` to the end of the URL and place `&` between each parameter and value, where you would usually put a space.  This would look something like:

    https://127.0.0.1/?-winsize&1152&864&+menu_load

Registered Version Detection
----------------------------

Like the original game, *Quake-WASM* requires `gfx/pop.lmp` to be present to detect if you're running the full version.  However, this version allows that file to contain any contents, or it can be 0 bytes in size.  You can add this folder and file yourself, if you wish.

Simply forcing the game to run registered by default is not a good idea because it:

- Breaks proper compatibility with the official shareware version, most notably when accessing levels in the *Introduction*.
- Potentially breaks third-party add-ons that use assets from the full version, if only the shareware files are present.

Recording Demos
---------------

Recording demos is supported.  You can start one by typing something like this in the console:

    skill 3
    record mydemo-e1m1 e1m1

To stop the demo:

    stop

The file will be saved in the browser database, but you will also be given the opportunity to export it if you want to.

The demo can be played back with `playdemo mydemo-e1m1`.

Networking Support
------------------

You can play in a browser window, but WebSockets support for multiplayer has not yet been added.

It should be possible to connect to a WebSockets proxy to enable online play, but the project will need rebuilding with the appropriate proxy configuration.

Building Quake from Source (on Linux)
-------------------------------------

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

Building GLQuake from Source (on Linux)
---------------------------------------

This is the same as building Quake, except you should additionally:

1. Download *GL4ES* and build it for Emscripten, as per its instructions.
2. Edit `Makefile.emscripten` to point to *GL4ES*'s `include` and `lib` directories.
3. Call `make -f Makefile.emscripten GLQUAKE=1` as a substitute for the final step.

Quake and GLQuake can co-exist on the same web server and will share settings / saves if on the same domain.

Notes
-----

When building *Quake-WASM*, all filenames included in the `id1` folder should be in lowercase.  The Quake engine is case-sensitive on anything other than Windows.
