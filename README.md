# FromStutterFixUpdated

This tool will apply a fix for a certain type of stutter in FromSoft games. It can also disable achievements in Elden Ring, to work around a bug where achievements can freeze the game when Steam is offline.

Supported patches:
* DS3 1.15.1
* Sekiro 1.06
* Elden Ring 1.10.1

## What's new?

* I have updated the Elden Ring pointers so it supports the latest patch and plan on keeping them updated into the future. I wanted to avoid the temptation of cheating with ERTool, so updated this instead.
* I still want to earn achievements so I've added an option to enable the achievement freeze fix. To enable it, add a file with no extension called "fix" in the same folder as the EXE file.
* Made it easy to re-run the program without having to close it.
* And it's just an EXE cos I dunno how to do DLLs and I like the re-run thingie.

## Usage

Run the game, then run the program. It will say "flag set" if it worked. Close the program. The fix will last until you restart the game.

## How does it work?

The game does...something, relating to Steam Input, which scans over all devices (basically everything listed in Device Manager). This happens any time any device changes, like a microphone being plugged in, a bluetooth headset pairing, or a driver update happening in the background. The game stops rendering frames until this is done, and it can take up to a second on some PCs.

The game has a flag which turns this off. This program sets the flag.

## Okay, but how do I know it's working?

Run the game without the fix, and try plugging/unplugging a USB dongle. If windowed, make sure the game window is focused (the stutter won't happen otherwise). You should see a noticeable stutter. Now apply the fix, then repeat the test. It should no longer occur.

## Does it break anything?

I don't know. I haven't noticed any problems. If you actually use Steam Input, it might stop working. Replugging a controller still works for me.

The achievement-disabling freeze fix will prevent you from getting any achievements, but you probably have them all by now anyway.

## This fixed my unplayable stutters that happened every 10 seconds!

Cool, but something is probably still wrong with your PC. Check if anything looks weird in Device Manager, eg. if it's constantly refreshing, or any warning symbols.

## I still get stutters at certain points in the game

Stutters at loading triggers probably can't be fixed, except by From, or a miracle patch to the rendering pipeline. There may be other sources of stutter also.

## Credit

* Original program is by kh0nsu, I have not done much beside add some basic features and change some numbers.
* I found the new pointers by debugging ERTool and just adding those to this program.