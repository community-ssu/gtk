#FIXME: should valgrind this test
from Numeric import array, allclose as eq
from Numeric import dot as numdot, zeros, innerproduct as numinnerproduct, \
     conjugate, size, array
from MLab import rand
from dotblas import dot, innerproduct, vdot
from operator import add

def numvdot(a,b):
    return numdot(conjugate(a.flat), b.flat)
TYPECODES = ['D', 'l', '1', 's', 'd', 'w', 'f', 'b']
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
shapes = [(), (), (7,), (7,), (7, 1), (1, 1), (1, 9), (9, 9), (9, 100),
          (200,100,3), (2, 3, 5), (200, 3, 5, 2)]

for type in TYPECODES:
    for a1, a2 in [(makeArray(s1, type),
                    makeArray(s2, type))
                   for s1, s2 in zip(shapes[:-1], shapes[1:])]:
        assert eq(numdot(a1, a2), dot(a1,a2)), "bad dot"
	# test vdot, but only for vectors
## 	for a in a1, a2:
## 	    if not (len(a.shape) == 1 or len(a.shape) == 2 and max(a.shape) == 1):
## 		break
## 	else:
        if size(a1) == size(a2):
	    assert eq(numvdot(a1, a2), vdot(a1,a2)),\
	    "bad vdot dot"
        if len(a2.shape) > 1:
            sh = list(a2.shape)
            sh[-2], sh[-1] =  sh[-1], sh[-2]
            #IPTHON BUG if this line is wrong, ipython's execption handling
            #breaks
            a2.shape = tuple(sh)
        assert eq(numinnerproduct(a1, a2), innerproduct(a1,a2)), \
               "bad innerproduct"
try: dot(lambda :3, lambda :4)
except TypeError: pass
else: assert 0, "illegal input not handled correctly"
try: dot([3,4], lambda :4)
except TypeError: pass
else: assert 0, "illegal input not handled correctly"
