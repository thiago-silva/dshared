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
from multiprocessing import Process
import sys

class P(Process):
    def __init__(self, pname, shared):
        super(P, self).__init__()
        self.pname = pname
        self.shared = shared

    def insert(self, num):
        key = self.pname + "_" + str(num)
        print self.pname + ": inserting a new entry: " + key
        self.shared[key] = md5.md5(key).hexdigest()

    def show(self):
        print self.pname + " entries [["
        for k,v in self.shared.iteritems():
            print "  " + self.pname + ": [" + k + "] = " + v
        print self.pname + "]]\n"

    def run(self):
        print '** running: ' + self.pname
        for i in range(0,9):
            time.sleep(3)
            self.insert(i)
            self.show()


dshared.init("my_shared_mem",2 ** 20)

shared = dshared.dict({"initial":"initial val"})

p1 = P("p1", shared)
p1.start()

p2 = P("p2", shared)
p2.start()

p1.join()
p2.join()
