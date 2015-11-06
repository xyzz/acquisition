# Acquisition [![Build Status](https://travis-ci.org/xyzz/acquisition.svg?branch=master)](https://travis-ci.org/xyzz/acquisition) [![Build status](https://ci.appveyor.com/api/projects/status/yutua4cn9cjv6wym?svg=true)](https://ci.appveyor.com/project/xyzz/acquisition)

Acquisition is an inventory management tool for [Path of Exile](https://www.pathofexile.com/).

It is written in C++, uses Qt widget toolkit and runs on Windows and Linux.

Check the [website](http://get.acquisition.today) for screenshots and video tutorials. You can download Windows setup packages from [the releases page](https://github.com/xyzz/acquisition/releases).

## Compiling/developing Acquisition

### Windows

On Windows you can use either Visual Studio or MinGW version of Qt Creator. Alternatively you can also use Visual Studio with Qt Add-in. Note that only Visual Studio 2013 Update 3 is supported.

### Linux

Either open `acquisition.pro` in Qt Creator and build or do `qmake && make`.

## Command line arguments

`--data-dir <path>`: set the path where Acquisition should save its data. By default it's `%localappdata%\acquisition` on Windows and `~/.local/share/acquisition` on Linux.

`--test`: run tests. Zero exit code on success, other values indicate errors.
