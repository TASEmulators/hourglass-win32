.wtf is the input movie format of Hourglass.
It stands for "Windows TAS File".

In [r60](https://code.google.com/p/hourglass-win32/source/detail?r=60) or later, Hourglass also recognizes the extension .hgm ("Hourglass Movie"), which can be used instead of .wtf if you wish.

## WTF file format description ##
A WTF file consists of a 1024-byte header followed by the input data.

### Header format ###
```
0x000: 4-byte signature: 0x66 0x54 0x77 0x02
0x004: 4-byte little-endian unsigned int: total number of input frames
0x008: 4-byte little-endian unsigned int: rerecord count
0x00C: 8-byte unterminated ASCII-encoded string: keyboard layout such as "00000409" for a US keyboard
0x014: 4-byte little-endian unsigned int: frames per second
0x018: 4-byte little-endian unsigned int: initial system clock time plus 1
0x01C: 4-byte little-endian unsigned int: CRC32 of the game executable
0x020: 4-byte little-endian unsigned int: size in bytes of the game executable
0x024: 48-byte null-terminated ASCII-encoded string: filename of game executable (excluding directories)
0x054: 16 4-byte little-endian unsigned ints: system clock time at power-of-two frame boundaries, used for desync detection. Ignored if zero. Introduced in r41.
0x094: 4-byte little-endian unsigned int: revision number of Hourglass that the movie was recorded with. If 0, the revision is assumed to be 51 (or 39 if the first clock time at 0x054 is also 0). Introduced in r57.
0x098: 160-byte null-terminated ASCII-encoded string: command line arguments that are provided to the game. Introduced in r60.
0x138: reserved bytes which should be 0, padding until end of 1024 byte header
0x400: beginning of frame data
```

### Input Data ###
Each frame consists of 8 bytes of keyboard data.

Each byte is a [virtual key code](http://msdn.microsoft.com/en-us/library/dd375731%28v=vs.85%29.aspx) such as VK\_ESCAPE indicating that a particular key was held on that frame. The first zero-value byte indicates that no other keys were held on that frame.

Frames where more than 8 different keys are held cannot be represented by the movie format. Nevertheless, attempting to press more than 8 keys at once will not cause desyncs, because Hourglass implements recording by saving each frame of input in movie format and then playing that frame of input back as if in playback mode.

DirectInput key codes are not stored separately, instead, they are generated based on the virtual key codes and the keyboard layout string that's stored in the current movie.