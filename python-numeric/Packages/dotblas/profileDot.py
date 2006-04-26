from Numeric import *
import Numeric
from MLab import rand
import dotblas
import _dotblas

import time

def timeCall(func, *args, **kwargs):
    """Return the time (in ms) it takes to call func."""
    start = time.time()
    func(*args, **kwargs)
    return time.time() - start



def underline(s, lineC='-'):
  """Return s + newline + enough '-' (or lineC if specified) to underline it
  and a final newline."""

  return s + "\n" + lineC * len(s) + "\n"


def oldDot(a,b):
    """That's what Numeric did before the matrixproduct function was
    introduced."""
    return innerproduct(a, swapaxes(b, -1, -2))


def nTimes(times, func, *args):
    for i in range(times):
        func(*args)

class AsciiTable:
    #FIXME: truncation
    def __init__(self, headingsL, columnWidthL=None, out=None):
        """Make an ascii table.
        
        'headingsL'    -- stores the headings.
        'columnWidthL' -- stores the length for each column (column
                          headings and columns are truncated if necessary), or
                          'None' which means that the length of the longest
                          heading is used instead (this is also the default
                          behavior if nothing is passed as columnWidthL).
        'out'          -- where the output is written to
        """

        self.columnWidthL = columnWidthL
        self.headingsL = headingsL
        self.out = out
        self.nColumns = len(self.headingsL)
        if self.columnWidthL is None:
            self.columnWidthL = [None] * self.nColumns
        assert len(self.columnWidthL) == len(self.headingsL)            
        self.longestHeading = max(map(len, self.headingsL))
        for i in range(self.nColumns):
            self.columnWidthL[i] = self.columnWidthL[i] or self.longestHeading
        self.formatStr = "| ".join([("%%-%ds" % columnWidth)
                                   for columnWidth in self.columnWidthL])
        print >> self.out, self.formatStr % tuple(self.headingsL)
        self.addRule()
    def addRow(self, itemL):
        """Add a row to the table.

        itemL -- a list of items for each column.
        
        """
        print >> self.out, self.formatStr % tuple(itemL)
    def addRule(self):
        """Add a horizontal rule to the table."""
        print >> self.out, "+-".join([ '-' * columnWidth
                                      for columnWidth in self.columnWidthL])
    
def makeArray(shape, typecode):
    if typecode == 'D':
        res = zeros(shape, typecode)
        res.imag = (rand(*shape) - 0.5) * 100
        res.real = (rand(*shape) - 0.5) * 100
    # XXX cheat for the small integer types to avoid overflows and hassles
    elif typecode in '1bw':
        res = array((rand(2,2)) * 10)
    else:
        res = array((rand(*shape) - 0.5) * 100).astype(typecode)
    return res

funclist = [
##     (_dotblas.MatrixProduct, "_dotblas.MatrixProduct",
##      "the new, BLAS/Atlas accelerated function (C)"),
    (dotblas.dot, "dotblas.dot",
     "BLAS dot with python wrapper"), 
##     (multiarray.matrixproduct, "matrixproduct",
##      "the C-function called by newer versions of Numeric"),
    (Numeric.dot, "Numeric.dot",
     "matrix product with its Numeric wrapper"),
##     (oldDot, "Old dot",
##      "what older versions of numeric used to do")
    ]


sizes  = [10, 100, 1000]
timesL = [1000, 100, 10]
print """
Explanation:
------------
"""
for func in funclist:
    f,short,long = func
    print  "%-20s == %s" % (short, long)

print """
interpretation of the timings:

for each entry

a*b is called %d times
a*A is called %d times
A*A is called %d times
""" % tuple(timesL)

for typecode in ['D', 'l', 'd', 'f']:
    print "TYPECODE: %s" % typecode
    print "===========\n"
    for size in sizes:
        print
        table = AsciiTable(["Function"] + \
                           map(lambda s: s % {"size":size},
                               ["a*b (%(size)dx1 * %(size)dx1)",
                                "A*a (%(size)dx%(size)d * %(size)dx1)",
                                "A*B (%(size)dx%(size)d * %(size)dx%(size)d)"]),
                           [max(map(len, [f[1] for f in funclist])), None,None,None])
        for func in funclist:
            a = makeArray((size,), typecode)
            b = makeArray((size,), typecode)
            A = makeArray((size, size), typecode)
            B = makeArray((size, size), typecode)
            dotees = [(a,b), (A,a), (A,B)]
            times = [timeCall(nTimes, timesL[i], func[0], dotees[i][0], dotees[i][1])
                     for i in range(len(dotees))]
            table.addRow([func[1]] + ["%10.5f" % tim for tim in times])
            table.addRule()
        
