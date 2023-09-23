# RCalc

<p align=center>
  <img src="./assets/icon.svg">
</p>

This is an implementation of a calculator using
[RPN](https://en.wikipedia.org/wiki/Reverse_Polish_notation). Calculations are
done using fixed point arithmetics, to ensure better accuracy than floating
point arithmetics (operations like division are still not so accurate). 

Drawing the GUI is facilitated using [raylib](https://www.raylib.com/).

## Getting started

### Compiling for Linux

Make sure you have [raylib](https://www.raylib.com/) installed on your system.
Then enter the repository directory and run:

```console
make
./rcalc
```

### Compiling for Android

There is an already compiled version of raylib for Android present in
`raylib/libraylib.a`. If you want to compile it on your own follow instructions from
raylib wiki: [Working for Android](https://github.com/raysan5/raylib/wiki/Working-for-Android).

Additional dependencies:
- OpenJDK - (I use version 17)
- Android SDK - guide to downloading it can be found in link above
- Android NDK - (same as above) - you have to provide the path to its installation
directory in `ANDROID_NDK` environmental variable
- Gradle - (I use Gradle 8.3)

Then run in repository directory run:

```console
ANDROID=1 make
```

NOTE: To install the application on your connected device you will need `adb`.

After aquiring `adb` run:

```console
make install
```
