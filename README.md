dshared
=======

A quick 'n dirty python extension providing shared memory dictionaries.

## Summary

This extension creates an `mmap` shared memory segment where python values can
be stored and accessed from distinct processes -- no IPC, pickling/wiring, not
even locking will get in your way.

**important** notes to keep in mind:

* Currently, None, ints/longs, strings, dicts and lists can be stored in the
  shared memory. Dicts and lists may be recursive.

* Class instances are work in progress.

* This code was not written with reliability in mind (and if you noticed there
  is no "tests" directory here, you should know that it does not belong in
  production environments).

* Last but not least, I do not intend to maintain this library, as it was
  written to fix a very particular and temporary problem (for instance, I
  didn't even need support for floating point values in the SM). So, in the
  case you find this extension somehow useful and wish to fork/improve it,
  feel free to let me know so I can refer visitors of this repository to your
  repository.

## List of dependencies

 * Python (I'm using 2.7.5)
 * boost libraries (in particular, `Boost.Interprocess` compatible with v. 1.54.0.1)
 * A C++ compiler and python's development files (headers, libraries, etc)

## Installation on Debian-like systems
 * `# aptitude install libboost-all-dev`
 * `# python setup.py install`

##
