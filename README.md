A reimplementation of the [Breakpad](https://code.google.com/p/google-breakpad/) Windows dump_syms tool.

[![Build Status](https://travis-ci.org/luser/dump_syms.svg?branch=master)](https://travis-ci.org/luser/dump_syms)

Prerequisites
=============

This project requires [gyp](https://code.google.com/p/gyp/) to build. On Ubuntu you can `apt-get install gyp`. On Windows you can use [Chromium's depot_tools](http://dev.chromium.org/developers/how-tos/install-depot-tools).

On Linux you'll also probably want [ninja](http://martine.github.io/ninja/). (Windows users can get ninja as part of the depot_tools above.)

Building
========
* Run gyp to generate build files
```
gyp -f ninja --depth=. ./dump_syms.gyp
```
 * To generate a Visual Studio project you can instead run:
```
gyp -f msvs --depth=. ./dump_syms.gyp
```
* Run ninja to build
```
ninja -C out/Default/
```
