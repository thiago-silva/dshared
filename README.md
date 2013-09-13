dshared
=======

A quick 'n dirty python extension providing shared memory dictionaries.

## Summary

This extension creates a shared memory segment where (some) python values can
be stored and accessed from distinct processes -- no IPC, no
pickling/serializing, no wiring, not even locking will get in your way. Plain
direct access to memory between parent/child processes. An uninteresting
example:

```python
import dshared
from multiprocessing import Process
dshared.init("mymem", 2 ** 10) #creates shared mem of size 2 ** 10

shm = dshared.col()

def in_process_1():
  shm['d'] = {'key':'val'}

def later_on_in_process_2():
   print shm['d']['key']

p = Process(target=in_process_1)
p.start()
p.join()
p = Process(target=later_on_in_process_2)
p.start()
p.join()
```

**important** notes to keep in mind:

* Currently, None, ints/longs, strings, dicts and lists can be stored in the
  shared memory. Dicts and lists may be recursive.

* Instances of user defined classes are victims of evil voodoo when assigned
  to a shared dictionary (and so are the objects it points to! Somethings are
  just too wrong to be left unsaid).

* This code was not written with reliability in mind -- *far* from it. And if
  you noticed there is no "tests" directory here, that should settle any crazy
  ideas related to putting this stuff in production environments.

* Last but not least, I do not intend to maintain or improve this library, as
  it was written to fix a very particular and temporary problem. So, in the
  case you find this extension somehow useful and wish to fork/improve it,
  feel free to let me know so I can refer visitors of this repository to your
  repository.

Use it at your own risk, bla bla bla...Now that you have been warned:

## List of dependencies

 * Python (I'm using 2.7.5)
 * boost libraries (in particular, `Boost.Interprocess` compatible with v. 1.54.0.1)
 * A C++ compiler and python's development files (headers, libraries, etc)

## Installation on Debian-like systems
 * `# aptitude install libboost-all-dev`
 * `# python setup.py install`

##
