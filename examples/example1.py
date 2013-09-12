# Copyright (c) 2013 Thiago B. L. Silva <thiago@metareload.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import dshared
import pprint
import time
import random
import md5
from multiprocessing import Process, Lock
import sys
import traceback

def my_safe_repr(object, context, maxlevels, level):
    typ = pprint._type(object)
    if typ is unicode:
        object = str(object)
    return pprint._safe_repr(object, context, maxlevels, level)

printer = pprint.PrettyPrinter(1,80,3)
printer.format = my_safe_repr

def pp(obj):
    return printer.pformat(obj)

class Foo():
    def __init__(self, num, owner):
        print 'Foo.__init()'
        self.owner = owner
        self.bar = num
    def __getattr__(self, name):
        print 'getattr:' + name
        return self.__dict__[name]

class P():
    def __init__(self, pname, lock, shared, readonly = False):
        self.lock = lock
        self.pname = pname
        self.shared = shared
        self.readonly = readonly

    def insert(self, num):
        self.lock.acquire()
        key = self.pname + "_" + str(num)
        rd = int(random.random() * 10) % 4
        if rd == 0:
            print self.pname + ": inserting an object entry: " + key
            self.shared[key] = Foo(num, self.pname)
        if rd == 1:
            print self.pname + ": inserting a dict entry: " + key
            self.shared[key] = {md5.md5(key).hexdigest() : int(random.random()*100)}
        if rd == 2:
            print self.pname + ": inserting a string entry: " + key
            self.shared[key] = md5.md5(key).hexdigest()
        if rd == 3:
            print self.pname + ": inserting a number entry: " + key
            self.shared[key] = int(random.random()*100)
        self.lock.release()

    def show(self):
        self.lock.acquire()
        print self.pname + " entries " + str(len(self.shared)) + ": [["
        for k,v in self.shared.iteritems():
            if hasattr(v,'__dict__'):
                print "  " + self.pname + ": [" + k + "] = Foo::" + pp(v.__dict__)
            else:
                print "  " + self.pname + ": [" + k + "] = " + pp(v)
        print self.pname + "]]\n"
        self.lock.release()

    def run(self):
        print '** running: ' + self.pname
        try :
            for i in range(0,20):
                if not self.readonly:
                    self.insert(i)
                self.show()
        except Exception as e:
            print e
            print traceback.format_exc()


dshared.init("my_shared_mem",2 ** 20)

if __name__ == "__main__":
    def r(name, lock, shared, readonly):
        P(name, lock, shared,readonly).run()

    shared = dshared.dict({})

    lock = Lock()
    p1 = Process(target=r, args=("p1", lock, shared, False))
    p1.start()

    p2 = Process(target=r, args=("p2", lock, shared, False))
    p2.start()
    p1.join()
    p2.join()

print "done!"
