# first-cc-gcc
A port of the [earliest C compiler](https://www.bell-labs.com/usr/dmr/www/primevalC.html) to modern GCC. The compiler outputs PDP-11 assembly code that can be compiled and run on a PDP-11 emulator. The compiler runs only in 32 bit mode as the original code assumes that the pointer size and word size are the same.

## Usage
To compile the compiler and run it simply do:
```shell
make
./cc examples/fizzbuzz.c > fizzbuzz.s
```
Note: if you get errors on missing "bits/libc-header-start.h" headers make sure you have the 32bit libc installed.

### Emulator
The hard part is to set up an emulator, transfer the file to it(!) and run the assembler. A very [early UNIX implementation](https://github.com/qrush/unix) based on SIMH is available. For Windows, there is also a [pre-built binary](http://sourceforge.net/project/downloading.php?group_id=204974&filename=Research-unixv1-0.3.exe&a=25520957)

I could not get the tape emulators working so ended up with a hacky solution to transfer files. The simulator lets you to log in via telnet, so the files are copied by starting up a text editor on the simulator and streaming the characters into it and then saving and closing the file.

Also, if you close the connection the session is lost, so it is important to keep the connection to the 
simulator alive with a hack using ncat.
```shell
# Start emulator
pdp11 simh.cfg
# Open a pipe to the simulator
# If you use the prebuilt Windows simulator, use port 12323
ncat -lk -p 5556 | ncat localhost 5555

# send login username to emulator
echo root | emulator/emucat
# copy file over by typing it into ed
emulator/cpfile fizzbuzz.s /fizzbz.s 
# call assembler and linker
emulator/emucc /fizzbz.s 
# execute the compiled program
echo a.out | emulator/emucat
```

Note that the file is called `fizzbz.s` on the emulator. This is because the UNIX used here handles 8 character long filenames only!

## Old C features 

This version of C is from around 1972. While the general syntax is pretty much the same as today,
there are tons of missing features:
- no preprocessor, no for loops
- even though there is a keyword for `float` and `double`, floating point calculations are not implemented, you can not even write a floating point literal
- the type system is very weak: pointers, chars, ints can be freely converted into one another
- types of the function parameters are not checked, anything can be passed to any function
- compound assignment operators are reversed, they are =+, =*
- only integer global variables can be defined, and the syntax is strange (see helloworld example)
- variable names can be of any length but only the first 8 characters are used; i.e. deadbeef1 and deadbeef2 are effectively the same variables
  

Interestingly, some features that were already existing in this early version:
- function pointers
- the ABI is nearly the same as today's 32 bit ABI
- `a[b]` is implemented as `*(a+b)`

