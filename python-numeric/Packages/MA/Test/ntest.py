import unittest, types, math
import MA, Numeric

def eq (a, b):
    "Test if the two sequences are essentially equal"
    return MA.allclose(a,b)
  
class BasicMATestCase (unittest.TestCase):
    def testeq (self):
        "Test the eq function"
        assert eq(3,3)
        assert not eq(3,4)
        assert eq([3,3,3],3)
        assert eq([2.,3.,4.], MA.array([2.,3.,4.]))

    def testBasicConstruction (self):
        "Test of basic construction."
        alist = [1,2,3]
        x = MA.array(alist)
        assert len(x) == 3
        assert x.typecode() == MA.Int
        assert x.spacesaver() == 0
        assert x.shape == (3,)
        assert eq (x,alist)
        y = MA.array(x, copy=0)
        assert x.raw_data() is y.raw_data()
        y[2] = 9
        assert x[2] == 9
        z = MA.array(x, savespace = 1, typecode = MA.Float)
        assert z.typecode() == MA.Float
        assert z.spacesaver() == 1
        x = MA.array([1,2,3.])
        assert x.typecode() == MA.Float
        x = MA.array([1,'who', 3.], MA.PyObject)
        assert x.typecode() == MA.PyObject
        w = MA.array([1,2], MA.Int32)
        assert w.itemsize() == 4
        assert w.iscontiguous()
        assert w.astype(MA.Float).typecode() == MA.Float

    def testTypecodeSpecifying(self):
        'Test construction using the type codes.'
        from Precision import typecodes
        thetypes = typecodes['Integer']  \
                   + typecodes['UnsignedInteger'] \
                   + typecodes['Float'] \
                   + typecodes['Complex'] 
        for t in thetypes:
            x = MA.array([1,2,3], t)
            assert x.typecode() == t
        x = MA.array(['hi', 'hoo'], 'c')
        assert x.typecode() == 'c'

    def testMultipleDimensionCreation (self):       
        'Test creation of multidimensional arrays.'
        alist = [[2,3],[4,5]]
        x = MA.array(alist)
        assert x.shape == (2,2)
        y = MA.array([x, x + 1, x + 2])
        assert y.shape == (3,2,2)

    def testIndexing (self):
        'Test indexing operations.'
        x = MA.array([0,1,2,3,4,5])
        for i in range(len(x)):
            assert i == x[i]
        x[2] = 20
        assert eq(x, x[...])
        w = MA.array([None])
        assert w.typecode() == MA.PyObject
        assert w[0] is None
        assert isinstance(x[2], types.IntType)
        assert x[2] == 20
        x = MA.array([0,1,2,3,4,5,6])
        assert eq (x[4:1:-1], [4,3,2])
        assert eq(x[4:1:-2], [4,2])
        assert eq(x[::-1], [6, 5,4,3,2,1,0])
        assert eq(x[2:-1], [2,3,4,5])
        m = MA.array([[1,2,3],[11,12,13]])
        assert m[0,2] == 3
        assert isinstance(m[0,2], types.IntType)
        assert eq(m[...,1], [2,12])
        assert eq(MA.arange(6)[..., MA.NewAxis], [[0],[1],[2],[3],[4],[5]])
        x = MA.array([1,2,3])
        y = MA.array(x)
        x[0] == 66
        assert y[0] != 66
        b=MA.array([[1,2,3,4],[5,6,7,8]]*2)
#        assert b[1:1].shape == (0,4)
#        assert b[1:1, :].shape == (0,4)
#        assert b[10:].shape == (0,4)
        assert eq(b[2:10], [[1,2,3,4],[5,6,7,8]])
        assert eq(b[2:10, ...], [[1,2,3,4],[5,6,7,8]])

    def testOperators (self):
        "Test the operators +, -, *, /, %, ^, &, |"
        x = MA.array([1.,2.,3.,4.,5.,6.])
        y = MA.array([-1.,2.,0.,2.,-1, 3.])
        assert eq(x + y, [0., 4., 3., 6., 4., 9.])
        assert eq(x - y, [2., 0., 3., 2., 6., 3.])
        assert eq(x * y, [-1., 4., 0., 8., -5., 18.])
        assert eq(y / x, [-1, 1., 0., .5, -.2, .5])
        assert eq(x**2, [1., 4., 9., 16., 25., 36.])
        xc = MA.array([1.,2.,3.,4.,5.,6.])
        xc += y
        assert eq(xc, x + y)
        xc = MA.array([1.,2.,3.,4.,5.,6.])
        xc -= y
        assert eq(xc, x - y)
        yc = MA.array(y, copy=1)
        yc /= x
        assert eq ( yc, y / x)
        xc = MA.array([1.,2.,3.,4.,5.,6.])
        y1 = [-1.,2.,0.,2.,-1, 3.]
        xc *= y1
        assert eq(xc, x * y1)

        assert eq (x + y, MA.add(x, y))
        assert eq (x - y, MA.subtract(x, y))
        assert eq (x * y, MA.multiply(x, y))
        assert eq (y / x, MA.divide (y, x))
        d = x / y
        assert d[2] is MA.masked 
        assert (MA.array(1) / MA.array(0)) is MA.masked
        assert eq (x**2, MA.power(x,2))
        x = MA.array([1,2])
        y = MA.zeros((2,))
        assert eq (x%x, y)
        assert eq (MA.remainder(x,x), y)
        assert eq (x <<1, [2,4])
        assert eq (MA.left_shift(x,1), [2,4])
        assert eq (x >>1, [0,1])
        assert eq (MA.right_shift(x,1), [0,1])
        assert eq (x & 2, [0,2])
        assert eq (MA.bitwise_and (x, 2), [0,2])
        assert eq (x | 1, [1,3])
        assert eq (MA.bitwise_or (x, 1), [1,3])
        assert eq (x ^ 2, [3,0])
        assert eq (MA.bitwise_xor(x,2), [3,0])
#        x = divmod(MA.array([2,1]), MA.array([1,2]))
#        assert eq (x[0], [2,0])
#        assert eq (x[1], [0,1])
        assert (4L*MA.arange(3)).typecode() == MA.PyObject

    def testComparisons (self):
        "Test rich comparisons."
        array = MA.array
        x = array([1,2,3])
        y = array([1,2,4])
        z = array([1,2,3,4])

        self.failUnlessRaises(ValueError, MA.equal, x, z)
        assert eq(x < y, array([0,0,1]))
        assert cmp(x,x) == 0
        assert eq(x == y, array([1,1,0]))
        assert eq(x >= y, array([1,1,0]))
        assert eq(x <= y, array([1,1,1]))
        assert eq(x > y, array([0,0,0]))
        # Test of rank-0 comparisons
        x = array(3,'s')
        y = array(4.0,'f')
        z = array(3.0,'f')
        assert (x < y) == 1
        assert (x > y) == 0
        assert (x == z) == 1
        
    def testComplex (self):
        "Test complex numbers"
        y = MA.arange(5) * 1.0
        x = y + y[::-1] * 1.0j
        assert x.typecode() == MA.Complex
        assert eq(x.real, y)
        assert eq(x.imag, y[::-1])

    def testReductions (self):
        "Tests of reduce attribute."
        a = MA.arange(6)
        m = MA.array([[1,2,3],[11,12,13]])
        assert MA.add.reduce(a) == 15
        assert MA.multiply.reduce(m.shape) == len(m.flat)
        assert eq(MA.add.reduce (m, 0), [12,14,16])
        assert eq(MA.add.reduce (m, -1), [6,36])
        assert eq(MA.multiply.reduce (m, 0), [11,24,39])
        assert eq(MA.multiply.reduce (m, -1), [6,11*12*13])
        assert MA.add.reduce([1]) == 1
        assert MA.add.reduce([]) == 0
        assert MA.multiply.reduce([]) == 1
        assert MA.minimum.reduce(a) == 0
        assert MA.maximum.reduce(a) == 5
        
    def testSpacesaver (self):
        "Test the spacesaver property (Travis Oliphant)"
        # Test of savespace property:  Travis Oliphant
        a = MA.array([1,2,3,4],savespace=1)
        assert a.spacesaver()
        self.assertEqual(a.typecode(), 's')
        b = MA.array(a,'f')
        self.assertEqual(b.typecode(), 'f')
        assert b.spacesaver()
        a.savespace(0)
        assert not a.spacesaver()
        assert b.spacesaver()
        d = 4 * b
        assert b.typecode() == d.typecode()
        self.failUnlessRaises, TypeError, MA.arccos, (b/10.0)

        
class ArrayCreatorsTestCase (unittest.TestCase):
    "Tests of ones, zeros, arange"
    def testOnes(self):
        "Test ones"
        y = MA.ones((2,3))
        assert y.shape == (2,3)
        assert y.typecode() == MA.Int
        assert eq(y.flat, 1)
        z = MA.ones((2,3), MA.Float)
        assert z.shape == (2,3)
        assert eq(y, z)
        w = MA.ones((2,3), MA.Int16)
        assert eq(w, MA.array([[1,1,1],[1,1,1]],'s'))
        self.failUnlessRaises(ValueError, MA.ones, (-5,))
        
    def testZeros(self):
        "Test zeros"
        y = MA.zeros((2,3))
        assert y.shape == (2,3)
        assert y.typecode() == MA.Int
        assert eq(y, 0)
        z = MA.zeros((2,3), MA.Float)
        assert z.shape == (2,3)
        assert eq (y, z)
        self.failUnlessRaises(ValueError, MA.zeros, (-5,))

    def testArange(self):
        "Test arange"
        x = MA.arange(5)
        assert eq(x, [0,1,2,3,4])
        x = MA.arange(4, 6)
        assert eq(x, [4,5])
        x = MA.arange(0, 1.5, .5)
        assert eq (x, [0.0, 0.5, 1.0])
        assert len(MA.arange(5,3,2)) == 0
        x = MA.arange(3L)
        assert x[1] == 1L
        assert x.typecode() == MA.PyObject

    def test_fromstring(self):
        "Test the fromstring function."
        x = MA.fromstring("\001\002\003", 'b')
        assert x.typecode() == 'b'
        assert eq (x, [1,2,3])

    def testFromfunction(self):
        "Test the fromfunction function."
        assert eq (MA.fromfunction(MA.add, (2,2)), [[0, 1],[1, 2]])


class ArrayFunctionsTestCase (unittest.TestCase):
    def setUp (self):
        self.a = MA.arange(6)
        self.m = MA.array([[1,2,3],[11,12,13]])

    def testTake (self):
        "test take"
        x = MA.take(self.a, (3,2,1,2,3))
        assert eq (x, [3,2,1,2,3])
        x = MA.take(self.m, (2,2,1,1), -1)
        assert eq(x, [[3,3,2,2],[13,13,12,12]])

    def testReshape (self):
        "test reshape"
        w = MA.reshape(self.m, (-1,2))
        assert eq(w, [[1,2],[3,11],[12,13]])
        w = MA.reshape (self.a, (2,3))
        assert eq(w, [[0, 1, 2], [3, 4, 5]])
        self.failUnlessRaises(ValueError, MA.reshape, MA.arange(5), (3,2))
        self.failUnlessRaises(ValueError, MA.reshape, self.a, (4,-1))
        self.failUnlessRaises(ValueError, MA.reshape, self.a, (5,6))

    def testRavel (self):
        "test ravel"
        assert MA.ravel(self.m).shape == (6,)
        assert eq (MA.ravel(self.m), [1,2,3,11,12,13])

    def testTranspose (self):
        "test transpose"
        assert not MA.transpose(self.m).iscontiguous()
        assert eq(MA.transpose(self.m), [[1,11], [2,12], [3,13]])
        assert eq(MA.transpose(MA.transpose(self.m)), self.m)

    def testRepeat (self):
        "test repeat"
        assert eq(MA.repeat(self.a, (0,1,2,3,0,0)), [1,2,2,3,3,3])
        assert MA.repeat(self.a, (0,)*6).typecode() == self.a.typecode()
        assert eq(MA.repeat(self.a, (0,)*6), [])
        assert eq(MA.repeat(self.m, (0,2,1), -1), [[2,2,3],[12,12,13]])

    def testConcatenate (self):
        "test concatenate"
        assert eq (MA.concatenate((self.a[:3], self.a[3:])), [0,1,2,3,4,5])
        m = self.m
        assert eq (MA.concatenate((m, m)), [[1,2,3], [11,12,13], [1,2,3], [11,12,13]])
        assert eq (MA.concatenate((m, m), axis = 1), 
                   [[1,2,3,1,2,3],[11,12,13,11,12,13]])

    def testPut (self):
        "test put and putmask"
        x=MA.arange(5)
        MA.put (x, [1,4],[10,40])
        assert eq(x, [0,10,2,3,40])
        
        x=MA.arange(5) * 1.0
        MA.put (x, [1,4],[10.,40.])
        assert eq(x, [0.,10.,2.,3.,40.])
        
        x=MA.arange(5) 
        MA.put (x, [1,4],[10])
        assert eq(x, [0,10,2,3,10])
        
        x=MA.arange(5) 
        MA.put (x, [1,4],10)
        assert eq(x, [0,10,2,3,10])
        
        x=MA.arange(5) 
        MA.put (x, [0,1,2,3], [10,20])
        assert eq(x, [10,20,10,20,4])
        
        x=MA.arange(5) 
        MA.put (x, [[0,1],[2,3]], [10,20])
        assert eq(x, [10,20,10,20,4])
        
        x = MA.arange(5).astype(MA.Float32)
        MA.put (x, [1,4],[10.,40.])
        assert eq(x, [0,10,2,3,40])
        
        x=MA.arange(6)*1.0
        x.shape=(2,3)
        MA.put(x, [1,4],[10,40])
        assert eq(x, [[0,10,2],[3,40,5]])
        
        x=MA.arange(5)
        MA.putmask (x, [1,0,1,0,1], [-1,10,20,30,40]) 
        assert eq(x, [-1,1,20,3,40])
        
        x=MA.arange(10)
        MA.putmask(x, MA.ones(10), 5)
        assert eq(x, 5*MA.ones(10))
        
        x=MA.arange(10)*1.0
        x=x.astype(MA.Float32)
        MA.putmask(x, [0,0,1,0,0,0,0,0,0,1], 3.0)
        assert eq(x, [0.,1.,3.,3.,4.,5.,6.,7.,8.,3.])
        
        x=MA.zeros((10,),MA.PyObject)
        MA.putmask(x, [0,0,1,0,0,0,1,0,0,1], 0.0)
        assert x[2] == 0.
        assert x[1] == 0
        
        x=MA.zeros((5,2),MA.PyObject)
        m=MA.zeros((5,2), MA.Int)
        m[3,1] = 1
        m[2,0] = 1
        MA.putmask(x, m, 0.0)
        assert x[3,1] == 0.0
        assert x[2,0] == 0

    def testDotOuter (self):
        "test the dot product and outer product"
        assert MA.dot(self.a, self.a) == 55
        assert eq (MA.add.outer(self.a[:3], self.a[:3]),
                   [[0,1,2],[1,2,3],[2,3,4]])
        assert eq (MA.outerproduct(self.a[:3], self.a[:3]),
                   [[0,0,0],[0,1,2],[0,2,4]])
        a = MA.arange(4)
        b = MA.arange(3)
        c = MA.zeros((4,3))
        c[0] = a[0]*b
        c[1] = a[1]*b
        c[2] = a[2]*b
        c[3] = a[3]*b
        assert eq(c, MA.outerproduct(a,b))

    def testChoose (self):
        "Test the choose function."
        assert eq (MA.choose (self.a, (5,4,3,2,1,0)), self.a[::-1])
        assert eq (MA.choose ([[1,0], [0,1]], (66, [(1,2),(11,12)])),
                   [[1,66],[66,12]])
        self.failUnlessRaises(ValueError, MA.choose, (0,1,2),([1,1,1],[2,2,2]))

    def testWhere (self):
        "Test the where function."
        assert eq(MA.where((0,1,2), [1,1,1],[2,2,2]), [2,1,1])

    def testSort (self):
        "Test sort, argsort, argmax, argmin"
        s = (3,2,5,1,4,0)
        sm = [s, MA.array(s)[::-1]]
        se = MA.array(s)[0:0]
        assert eq(MA.sort(s), self.a)
        assert len(MA.sort(se)) == 0
        assert eq(MA.argsort(s), [5,3,1,0,4,2])
        assert len(MA.argsort(se)) == 0
        assert eq(MA.sort(sm, axis = -1), [[0,1,2,3,4,5],[0,1,2,3,4,5]])
        assert eq(MA.sort(sm, axis = 0), [[0,2,1,1,2,0],[3,4,5,5,4,3]])
        assert MA.argmax(s) == 2
        assert MA.argmin(s) == 5
        assert eq(MA.argmax(sm, axis=-1), [2,3])
        assert eq(MA.argmax(sm, axis=1), [2,3])
        assert eq(MA.argmax(sm, axis=0), [0,1,0,1,0,1])
        assert eq(MA.argmin(sm, axis=-1), [5,0])
        assert eq(MA.argmin(sm, axis=1), [5,0])

    def testDiagonal (self):
        "Test the diagonal function."
        b=MA.array([[1,2,3,4],[5,6,7,8]]*2)
        assert eq(MA.diagonal(b), [1,6,3,8])
        assert eq(MA.diagonal(b, -1), [5,2,7])
        c = MA.array([b,b])
        assert eq(MA.diagonal(c,1), [[2,7,4], [2,7,4]])
        
class UniversalFunctionsTestCase (unittest.TestCase):
    def setUp (self):
        self.a = .01 + MA.arange(6) / 8.0 
        self.m = MA.array([[1,2,3],[11,12,13]]) / 16.0
      
    def testTrig (self):
        "Test sin, cos, tan, and their inverses"
        assert eq (MA.arccos(-1.0), MA.pi)
        assert eq (MA.sin(self.a), map(math.sin, self.a))
        assert eq (MA.cos(self.a), map(math.cos, self.a))
        assert eq (MA.tan(self.a), map(math.tan, self.a))
        assert eq (MA.arccos(self.a), map(math.acos, self.a))
        assert eq (MA.arcsin(self.a), map(math.asin, self.a))
        assert eq (MA.arctan(self.a), map(math.atan, self.a))
        assert MA.sin(self.m).shape == self.m.shape
        assert MA.cos(self.m).shape == self.m.shape
        assert MA.tan(self.m).shape == self.m.shape
        assert MA.arcsin(self.m).shape == self.m.shape
        assert MA.arccos(self.m).shape == self.m.shape
        assert MA.arctan(self.m).shape == self.m.shape
        assert eq (MA.sin(self.m).flat, map(math.sin, self.m.flat))
        assert eq (MA.cos(self.m).flat, map(math.cos, self.m.flat))
        assert eq (MA.tan(self.m).flat, map(math.tan, self.m.flat))
        assert eq (MA.arcsin(self.m).flat, map(math.asin, self.m.flat))
        assert eq (MA.arccos(self.m).flat, map(math.acos, self.m.flat))
        assert eq (MA.arctan(self.m).flat, map(math.atan, self.m.flat))

    def testStandard (self):
        "Test sqrt, log, log10, exp"
        assert eq (MA.exp(1.0), MA.e)
        assert eq (MA.sqrt(self.a), map(math.sqrt, self.a))
        assert eq (MA.log(self.a), map(math.log, self.a))
        assert eq (MA.log10(self.a), map(math.log10, self.a))
        assert eq (MA.exp(self.a), map(math.exp, self.a))
        assert eq (MA.sqrt(self.m).flat, map(math.sqrt, self.m.flat))
        assert eq (MA.log(self.m).flat, map(math.log, self.m.flat))
        assert eq (MA.log10(self.m).flat, map(math.log10, self.m.flat))
        assert eq (MA.exp(self.m).flat, map(math.exp, self.m.flat))

    def testLogical (self):
        "Test logical_and, logical_or, sometrue, alltrue"
        x = MA.array([1,1,0,0])
        y = MA.array([1,0,1,0])
        assert eq(MA.logical_and (x,y), [1,0,0,0])
        assert eq(MA.logical_or (x,y), [1,1,1,0])
        assert MA.sometrue(x)
        assert not MA.sometrue(MA.zeros((3,)))
        assert MA.alltrue(MA.ones((3,)))
        assert not MA.alltrue(x)

class ArrayRelationsToOtherModulesTestCase (unittest.TestCase):
    def testPickle (self):
        "Test pickling of MA arrays."
        import pickle
        x = MA.arange(10)
        fpik = open('foo.pik', 'wb')
        pickle.dump(x, fpik, 0)
        fpik.close()
        fpik = open('foo.pik', 'rb')
        y = pickle.load(open('foo.pik', 'rb'))
        fpik.close()
        assert eq(y,[0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
        assert MA.alltrue(MA.equal(x,y))
        assert MA.sometrue(MA.equal(x,3))
        assert y.shape == (10,)

    def copyTest (self):
        "Test how MA works with the copy module."
        import copy
        x = MA.array([1,2,3])
        y = [1, x, 3]
        c1 = copy.copy(x)
        assert MA.allclose(x,c1)
        x[1] = 4
        assert not MA.allclose(x,c1)
        c2 = copy.copy(y)
        assert id(c2[1]) == id(x)
        c3 = copy.deepcopy(y)
        assert id(c3[1]) != id(x)
        assert MA.allclose(c3[1], x)
        
def suite():
    testsuite = unittest.makeSuite(BasicMATestCase, 'test')
    testsuite.addTest(unittest.makeSuite(ArrayCreatorsTestCase, 'test'))
    testsuite.addTest(unittest.makeSuite(ArrayFunctionsTestCase, 'test'))
    testsuite.addTest(unittest.makeSuite(UniversalFunctionsTestCase, 'test'))
    testsuite.addTest(unittest.makeSuite(ArrayRelationsToOtherModulesTestCase, 'test'))
    return testsuite

if __name__ == '__main__':
    r = unittest.main()
