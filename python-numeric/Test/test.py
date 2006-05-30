import unittest, os, types, math
import Numeric

def has_length(a):
    try:
        len(a)
    except:
        return 0
    else:
        return 1

def eq (a, b):
    "Test if the two sequences are essentially equal"
    seq_a = has_length(a)
    seq_b = has_length(b)
    if (seq_a and not seq_b) or (not seq_a and seq_b):
        raise ValueError(
            "trying to compare a sequence and non-sequence: a=%r b=%r" %
            (a,b))
    aa = Numeric.asarray(a)
    ab = Numeric.asarray(b)
    if aa.shape != ab.shape:
        raise ValueError("sequences have different shapes:\na%s=%r\nb%s=%r" %
                         (aa.shape, a, ab.shape, b))
    return Numeric.allclose(aa,ab)

def assert_eq(a,b):
    assert eq(a,b)
def assert_not_eq(a,b):
    try:
        equal = eq(a,b)
    except ValueError:
        return True
    return not equal

class BasicNumericTestCase (unittest.TestCase):
    def testeq (self):
        "Test the eq function"
        assert_eq(3,3)
        assert_not_eq(3,4)
        assert_not_eq([3,3,3],3)
        assert_eq([2.,3.,4.], Numeric.array([2.,3.,4.]))

    def testBasicConstruction (self):
        "Test of basic construction."
        alist = [1,2,3]
        x = Numeric.array(alist)
        assert len(x) == 3
        assert x.typecode() == Numeric.Int
        assert x.spacesaver() == 0
        assert x.shape == (3,)
        assert_eq(x,alist)
        y = Numeric.array(x, copy=0)
        assert x is y
        y[2] = 9
        assert x[2] == 9
        z = Numeric.array(x, savespace = 1, typecode = Numeric.Float)
        assert z.typecode() == Numeric.Float
        assert z.spacesaver() == 1
        x = Numeric.array([1,2,3.])
        assert x.typecode() == Numeric.Float
        x = Numeric.array([1,'who', 3.], Numeric.PyObject)
        assert x.typecode() == Numeric.PyObject
        w = Numeric.array([1,2], Numeric.Int32)
        assert w.itemsize() == 4
        assert w.iscontiguous()
        assert w.astype(Numeric.Float).typecode() == Numeric.Float
        assert Numeric.shape(alist) == Numeric.shape(x)
        assert Numeric.rank(x) == 1
        assert Numeric.rank(1) == 0
        assert Numeric.size(x) == len(alist)
        x2 = Numeric.array([x,x])
        assert Numeric.rank(x2) == 2
        assert Numeric.size(x2, 0) == 2
        assert Numeric.size(x2, 1) == len(x)
        assert Numeric.size(x2) == 2 * len(x)
        assert_eq(Numeric.average(alist), 2.0)

    def testTypecodeSpecifying(self):
        'Test construction using the type codes.'
        from Precision import typecodes
        thetypes = typecodes['Integer']  \
                   + typecodes['UnsignedInteger'] \
                   + typecodes['Float'] \
                   + typecodes['Complex'] 
        for t in thetypes:
            x = Numeric.array([1,2,3], t)
            assert x.typecode() == t
        x = Numeric.array(['hi', 'hoo'], 'c')
        assert x.typecode() == 'c'

    def testMultipleDimensionCreation (self):       
        'Test creation of multidimensional arrays.'
        alist = [[2,3],[4,5]]
        x = Numeric.array(alist)
        assert x.shape == (2,2)
        y = Numeric.array([x, x + 1, x + 2])
        assert y.shape == (3,2,2)

    def testIndexing (self):
        'Test indexing operations.'
        x = Numeric.array([0,1,2,3,4,5])
        for i in range(len(x)):
            assert i == x[i]
        x[2] = 20
        assert_eq(x, x[...])
        w = Numeric.array([None])
        assert w.typecode() == Numeric.PyObject
        assert w[0] is None
        assert isinstance(x[2], types.IntType)
        assert x[2] == 20
        x = Numeric.array([0,1,2,3,4,5,6])
        assert_eq(x[4:1:-1], [4,3,2])
        assert_eq(x[4:1:-2], [4,2])
        assert_eq(x[::-1], [6, 5,4,3,2,1,0])
        assert_eq(x[2:-1], [2,3,4,5])
        m = Numeric.array([[1,2,3],[11,12,13]])
        assert m[0,2] == 3
        assert isinstance(m[0,2], types.IntType)
        assert_eq(m[...,1], [2,12])
        assert_eq(Numeric.arange(6)[..., Numeric.NewAxis], [[0],[1],[2],[3],[4],[5]])
        x = Numeric.array([1,2,3])
        y = Numeric.array(x)
        x[0] == 66
        assert y[0] != 66
        b=Numeric.array([[1,2,3,4],[5,6,7,8]]*2)
        assert b[1:1].shape == (0,4)
        assert b[1:1, :].shape == (0,4)
        assert b[10:].shape == (0,4)
        assert_eq(b[2:10], [[1,2,3,4],[5,6,7,8]])
        assert_eq(b[2:10, ...], [[1,2,3,4],[5,6,7,8]])

        x = Numeric.array([5,6,7,8])
        a = Numeric.array([0,1,2,3], Numeric.Int32)
        n = a[-1]
        assert x[n] == 8

    def testSlice(self):
        "Test slicing, like x[1:3]"
        a = Numeric.array([1,2,3,4])
        assert_eq(a[0:], a)
        assert_eq(a[1:], [2,3,4])
        assert_eq(a[2:], [3,4])
        assert_eq(a[3:], [4])
        assert_eq(a[4:], [])
        assert_eq(a[5:], [])
        assert_eq(a[1:2], [2])
        assert_eq(a[1:3], [2,3])
        assert_eq(a[2:6], [3,4])
        assert_eq(a[-1:], [4])
        assert_eq(a[-2:], [3,4])
        assert_eq(a[-3:], [2,3,4])
        assert_eq(a[-4:], [1,2,3,4])
        assert_eq(a[-5:], [1,2,3,4])

    def testBackwardStrides (self):
        "Test backward stride operations like x[3::-1]"
        m = Numeric.array([1.,2.,3.,4.])
        assert_eq(m[2:1:-1], [3.])
        assert_eq(m[2::-1], [3.,2.,1.])
        assert_eq(m[1::-1], [2.,1.])
        assert_eq(m[4::-1], m[3::-1])
        assert_eq(m[4::-2], [4.,2.])
        assert_eq(m[-2:-2:-1], [])

    def testOperators (self):
        "Test the operators +, -, *, /, %, ^, &, |"
        x = Numeric.array([1.,2.,3.,4.,5.,6.])
        y = Numeric.array([-1.,2.,0.,2.,-1, 3.])
        assert_eq(x + y, [0., 4., 3., 6., 4., 9.])
        assert_eq(x - y, [2., 0., 3., 2., 6., 3.])
        assert_eq(x * y, [-1., 4., 0., 8., -5., 18.])
        assert_eq(y / x, [-1, 1., 0., .5, -.2, .5])
        assert_eq(x**2, [1., 4., 9., 16., 25., 36.])
        xc = Numeric.array([1.,2.,3.,4.,5.,6.])
        xc += y
        assert_eq(xc, x + y)
        xc = Numeric.array([1.,2.,3.,4.,5.,6.])
        xc -= y
        assert_eq(xc, x - y)
        xc = Numeric.array([1.,2.,3.,4.,5.,6.])
        xc *= y
        assert_eq(xc, x * y)
        yc = Numeric.array(y)
        yc /= x
        assert_eq( yc, y / x)
        assert_eq(x + y, Numeric.add(x, y))
        assert_eq(x - y, Numeric.subtract(x, y))
        assert_eq(x * y, Numeric.multiply(x, y))
        assert_eq(y / x, Numeric.divide (y, x))
        self.failUnlessRaises(ZeroDivisionError, Numeric.divide, 
                              Numeric.array(1), Numeric.array(0))
        assert_eq(x**2, Numeric.power(x,2))
        x = Numeric.array([1,2])
        y = Numeric.zeros((2,))
        assert_eq(x%x, y)
        assert_eq(Numeric.remainder(x,x), y)
        assert_eq(x <<1, [2,4])
        assert_eq(Numeric.left_shift(x,1), [2,4])
        assert_eq(x >>1, [0,1])
        assert_eq(Numeric.right_shift(x,1), [0,1])
        assert_eq(x & 2, [0,2])
        assert_eq(Numeric.bitwise_and (x, 2), [0,2])
        assert_eq(x | 1, [1,3])
        assert_eq(Numeric.bitwise_or (x, 1), [1,3])
        assert_eq(x ^ 2, [3,0])
        assert_eq(Numeric.bitwise_xor(x,2), [3,0])
        x = divmod(Numeric.array([2,1]), Numeric.array([1,2]))
        assert_eq(x[0], [2,0])
        assert_eq(x[1], [0,1])
        assert (4L*Numeric.arange(3)).typecode() == Numeric.PyObject
        x = Numeric.array([1,2,3,4,5,6],'u')
        y = Numeric.array([1,2,0,2,2,3],'u')
        assert_eq(x + y, [2, 4, 3, 6, 7, 9])
        assert_eq(x - y, [0, 0, 3, 2, 3, 3])
        assert_eq(x * y, [1, 4, 0, 8, 10, 18])
        assert_eq(y / x, [1, 1, 0, 0, 0, 0])
        assert_eq(y // x, [1, 1, 0, 0, 0, 0])
        assert_eq(x**2, [1, 4, 9, 16, 25, 36])

    def testComparisons (self):
        "Test rich comparisons."
        array = Numeric.array
        x = array([1,2,3])
        y = array([1,2,4])
        z = array([1,2,3,4])

        self.failUnlessRaises(ValueError, Numeric.equal, x, z)
        assert_eq(x < y, array([0,0,1]))
        assert cmp(x,x) == 0
        assert_eq(x == y, array([1,1,0]))
        assert_eq(x >= y, array([1,1,0]))
        assert_eq(x <= y, array([1,1,1]))
        assert_eq(x > y, array([0,0,0]))
        # Test of rank-0 comparisons
        x = array(3,'s')
        y = array(4.0,'f')
        z = array(3.0,'f')
        assert (x < y) == 1
        assert (x > y) == 0
        assert (x == z) == 1
        # Test of comparisons to None
        assert (Numeric.equal(None,None) == 1)
        assert (Numeric.equal(x, None) == 0)
        assert (Numeric.equal(None, x) == 0)
        assert (x == {}) == 0
        assert_eq(array([1,2,None,'abc'],'O')==array([1,3,None, None],'O'),
                        [1,0,1,0])
        assert_eq(array([1,2,None,'abc'],'O')==None,
                        [0,0,1,0])
        assert_eq(array([1,2,None,'abc'],'O')==[1,3,None, None],
                        [1,0,1,0])

    def testComplex (self):
        "Test complex numbers"
        y = Numeric.arange(5) * 1.0
        x = y + y[::-1] * 1.0j
        assert x.typecode() == Numeric.Complex
        assert_eq(x.real, y)
        assert_eq(x.imag, y[::-1])

    def testReductions (self):
        "Tests of reduce attribute."
        a = Numeric.arange(6)
        m = Numeric.array([[1,2,3],[11,12,13]])
        assert Numeric.add.reduce(a) == 15
        assert Numeric.multiply.reduce(m.shape) == len(m.flat)
        assert_eq(Numeric.add.reduce (m, 0), [12,14,16])
        assert_eq(Numeric.add.reduce (m, -1), [6,36])
        assert_eq(Numeric.multiply.reduce (m, 0), [11,24,39])
        assert_eq(Numeric.multiply.reduce (m, -1), [6,11*12*13])
        assert Numeric.add.reduce([1]) == 1
        assert Numeric.add.reduce([]) == 0
        assert Numeric.multiply.reduce([]) == 1
        assert Numeric.minimum.reduce(a) == 0
        assert Numeric.maximum.reduce(a) == 5

    def testAverage (self):
        "Test of average function."
        a = Numeric.arange(6)
        assert_eq(Numeric.average(a), 15.0/6.0)
        assert_eq(Numeric.average(a, weights=[1,1,1,2,1,0]), 
                  (0*1+1*1+2*1+3*2+4*1+5*0)/(1+1+1+2+1+0.0))
        assert_eq(Numeric.average(a, weights=[1,1,1,2,1,0],returned=1)[0], 
                  (0*1+1*1+2*1+3*2+4*1+5*0)/(1+1+1+2+1+0.0))
        assert_eq(Numeric.average(a, weights=[1,1,1,2,1,0],returned=1)[1], 
                  1+1+1+2+1+0)
        b = Numeric.arange(6) * 3
        assert_eq(Numeric.average([a,b]), (a + b) / 2.)    
        assert_eq(Numeric.average([a,b],1), [15/6.0, 45/6.0])   
        assert_eq(Numeric.average([a,b], axis=0, weights=[3,1]), (3.0*a+b)/4.)
        w = Numeric.ones(Numeric.shape([a,b]), Numeric.Float)/2.0
        assert_eq(Numeric.average([a,b], axis=1), 
                  Numeric.average([a,b], weights=w, axis=1)
                  )
        c = a[:]
        c.shape=(3,2)
        assert_eq(Numeric.average(a), Numeric.average(c, axis=None))
        assert_eq(Numeric.average(a[::2]), (0+2+4)/3.0)
        r1, w1 = Numeric.average([[a,b],[b,a]], axis=1, returned=1)
        assert Numeric.shape(r1) == Numeric.shape(w1)
        assert r1.shape == w1.shape
        r2, w2 = Numeric.average(Numeric.ones((2,2,3)), axis=0, weights=[3,1], returned=1)
        assert Numeric.shape(w2) == Numeric.shape(r2)
        r2, w2 = Numeric.average(Numeric.ones((2,2,3)), returned=1)
        assert Numeric.shape(w2) == Numeric.shape(r2)
        r2, w2 = Numeric.average(Numeric.ones((2,2,3)), weights=Numeric.ones((2,2,3)), returned=1)
        assert Numeric.shape(w2) == Numeric.shape(r2)

    def testStr(self):
        a = Numeric.array('test', typecode=Numeric.Character)
        assert str(a) == '[t e s t]'

    def testSpacesaver (self):
        "Test the spacesaver property (Travis Oliphant)"
        # Test of savespace property:  Travis Oliphant
        a = Numeric.array([1,2,3,4],savespace=1)
        assert a.spacesaver()
        assert a.typecode() == 's'
        b = Numeric.array(a,'f')
        assert b.typecode() == 'f'
        assert b.spacesaver()
        a.savespace(0)
        assert not a.spacesaver()
        assert b.spacesaver()
        d = 4 * b
        assert b.typecode() == d.typecode()
        self.failUnlessRaises, TypeError, Numeric.arccos, (b/10.0)

    def testObject(self):
        "Test various object arrays"
        import UserString
        f,b,w = (map(UserString.UserString, "foo bar watz".split()))
        a = Numeric.array([f,w,b],'O')
        b = Numeric.array([f,w,b])
        a == f
        self.failUnlessRaises, ValueError, Numeric.not_equal.reduce, [None,"ABC","DEF"]


        
class ArrayCreatorsTestCase (unittest.TestCase):
    "Tests of ones, zeros, arange"
    def testOnes(self):
        "Test ones"
        y = Numeric.ones((2,3))
        assert y.shape == (2,3)
        assert y.typecode() == Numeric.Int
        assert_eq(y.flat, [1,1,1,1,1,1])
        z = Numeric.ones((2,3), Numeric.Float)
        assert z.shape == (2,3)
        assert_eq(y, z)
        w = Numeric.ones((2,3), Numeric.Int16)
        assert_eq(w, Numeric.array([[1,1,1],[1,1,1]],'s'))
        self.failUnlessRaises(ValueError, Numeric.ones, (-5,))
        
    def testZeros(self):
        "Test zeros"
        y = Numeric.zeros((2,3))
        assert y.shape == (2,3)
        assert y.typecode() == Numeric.Int
        assert_eq(y, [[0,0,0],[0,0,0]])
        z = Numeric.zeros((2,3), Numeric.Float)
        assert z.shape == (2,3)
        assert_eq(y, z)
        self.failUnlessRaises(ValueError, Numeric.zeros, (-5,))

    def testArange(self):
        "Test arange"
        x = Numeric.arange(5)
        assert_eq(x, [0,1,2,3,4])
        x = Numeric.arange(4, 6)
        assert_eq(x, [4,5])
        x = Numeric.arange(0, 1.5, .5)
        assert_eq(x, [0.0, 0.5, 1.0])
        assert len(Numeric.arange(5,3,2)) == 0
        x = Numeric.arange(3L)
        assert x[1] == 1L
        assert x.typecode() == Numeric.PyObject

    def test_fromstring(self):
        "Test the fromstring function."
        x = Numeric.fromstring("\001\002\003", 'b')
        assert x.typecode() == 'b'
        assert_eq(x, [1,2,3])

    def testFromfunction(self):
        "Test the fromfunction function."
        assert_eq(Numeric.fromfunction(Numeric.add, (2,2)), [[0, 1],[1, 2]])


class ArrayFunctionsTestCase (unittest.TestCase):
    def setUp (self):
        self.a = Numeric.arange(6)
        self.m = Numeric.array([[1,2,3],[11,12,13]])

    def testTake (self):
        "test take"
        x = Numeric.take(self.a, (3,2,1,2,3))
        assert_eq(x, [3,2,1,2,3])
        x = Numeric.take(self.m, (2,2,1,1), axis=-1)
        assert_eq(x, [[3,3,2,2],[13,13,12,12]])

    def testReshape (self):
        "test reshape"
        w = Numeric.reshape(self.m, (-1,2))
        assert_eq(w, [[1,2],[3,11],[12,13]])
        w = Numeric.reshape (self.a, (2,3))
        assert_eq(w, [[0, 1, 2], [3, 4, 5]])
        self.failUnlessRaises(ValueError, Numeric.reshape, Numeric.arange(5), (3,2))
        self.failUnlessRaises(ValueError, Numeric.reshape, self.a, (4,-1))
        self.failUnlessRaises(ValueError, Numeric.reshape, self.a, (5,6))

    def testRavel (self):
        "test ravel"
        assert Numeric.ravel(self.m).shape == (6,)
        assert_eq(Numeric.ravel(self.m), [1,2,3,11,12,13])

    def testTranspose (self):
        "test transpose"
        assert not Numeric.transpose(self.m).iscontiguous()
        assert_eq(Numeric.transpose(self.m), [[1,11], [2,12], [3,13]])
        assert_eq(Numeric.transpose(Numeric.transpose(self.m)), self.m)

    def testRepeat (self):
        "test repeat"
        assert_eq(Numeric.repeat(self.a, (0,1,2,3,0,0)), [1,2,2,3,3,3])
        assert Numeric.repeat(self.a, (0,)*6).typecode() == self.a.typecode()
        assert_eq(Numeric.repeat(self.a, (0,)*6), [])
        assert_eq(Numeric.repeat(self.m, (0,2,1), -1), [[2,2,3],[12,12,13]])

    def testConcatenate (self):
        "test concatenate"
        assert_eq(Numeric.concatenate((self.a[:3], self.a[3:])), [0,1,2,3,4,5])
        m = self.m
        assert_eq(Numeric.concatenate((m, m)), [[1,2,3], [11,12,13], [1,2,3], [11,12,13]])
        assert_eq(Numeric.concatenate((m, m), axis = 1), 
                   [[1,2,3,1,2,3],[11,12,13,11,12,13]])

    def testPut (self):
        "test put and putmask"
        x=Numeric.arange(5)
        Numeric.put (x, [1,4],[10,40])
        assert_eq(x, [0,10,2,3,40])
        
        x=Numeric.arange(5) * 1.0
        Numeric.put (x, [1,4],[10.,40.])
        assert_eq(x, [0.,10.,2.,3.,40.])
        
        x=Numeric.arange(5) 
        Numeric.put (x, [1,4],[10])
        assert_eq(x, [0,10,2,3,10])
        
        x=Numeric.arange(5) 
        Numeric.put (x, [1,4],10)
        assert_eq(x, [0,10,2,3,10])
        
        x=Numeric.arange(5) 
        Numeric.put (x, [0,1,2,3], [10,20])
        assert_eq(x, [10,20,10,20,4])
        
        x=Numeric.arange(5) 
        Numeric.put (x, [[0,1],[2,3]], [10,20])
        assert_eq(x, [10,20,10,20,4])
        
        x = Numeric.arange(5).astype(Numeric.Float32)
        Numeric.put (x, [1,4],[10.,40.])
        assert_eq(x, [0,10,2,3,40])
        
        x=Numeric.arange(6)*1.0
        x.shape=(2,3)
        Numeric.put(x, [1,4],[10,40])
        assert_eq(x, [[0,10,2],[3,40,5]])
        
        x=Numeric.arange(5)
        Numeric.putmask (x, [1,0,1,0,1], [-1,10,20,30,40]) 
        assert_eq(x, [-1,1,20,3,40])
        
        x=Numeric.arange(10)
        Numeric.putmask(x, Numeric.ones(10), 5)
        assert_eq(x, 5*Numeric.ones(10))
        
        x=Numeric.arange(10)*1.0
        x=x.astype(Numeric.Float32)
        Numeric.putmask(x, [0,0,1,0,0,0,0,0,0,1], 3.0)
        assert_eq(x, [0.,1.,3.,3.,4.,5.,6.,7.,8.,3.])
        
        x=Numeric.zeros((10,),Numeric.PyObject)
        Numeric.putmask(x, [0,0,1,0,0,0,1,0,0,1], 0.0)
        assert x[2] == 0.
        assert x[1] == 0
        
        x=Numeric.zeros((5,2),Numeric.PyObject)
        m=Numeric.zeros((5,2), Numeric.Int)
        m[3,1] = 1
        m[2,0] = 1
        Numeric.putmask(x, m, 0.0)
        assert x[3,1] == 0.0
        assert x[2,0] == 0

    def testDotOuter (self):
        "test the dot product and outer product"
        assert Numeric.dot(self.a, self.a) == 55
        assert_eq(Numeric.add.outer(self.a[:3], self.a[:3]),
                   [[0,1,2],[1,2,3],[2,3,4]])
        assert_eq(Numeric.outerproduct(self.a[:3], self.a[:3]),
                   [[0,0,0],[0,1,2],[0,2,4]])
        a = Numeric.arange(4)
        b = Numeric.arange(3)
        c = Numeric.zeros((4,3))
        c[0] = a[0]*b
        c[1] = a[1]*b
        c[2] = a[2]*b
        c[3] = a[3]*b
        assert_eq(c, Numeric.outerproduct(a,b))
        a = Numeric.arange(8)
        b = a[::2]
        c = Numeric.outerproduct(b,b)
        assert_eq(c, Numeric.outerproduct([0,2,4,6],[0,2,4,6]))

    def testDotZeroSize(self):
        '''Test dot product with 0-sized arrays
        Bug #1151892'''
        A = Numeric.zeros((0,0), Numeric.Float64)
        b = Numeric.zeros((0,), Numeric.Float64)
        x = Numeric.dot(A, b)
        assert_eq(x, Numeric.zeros( (0,), Numeric.Float64))
        B = Numeric.zeros((0,0), Numeric.Float64)
        X = Numeric.dot(A, B)
        assert_eq(X, Numeric.zeros( (0,0), Numeric.Float64))

    def testCrossProduct(self):
        """Test the cross_product function"""
        a = Numeric.array([1,2,3])
        b = Numeric.array([4,5,6])
        assert_eq(Numeric.cross_product(a,b), [-3, 6, -3])
        a = Numeric.array([1,2])
        b = Numeric.array([4,5])
        assert_eq(Numeric.cross_product(a,b), -3)
        a = Numeric.array([[1,2,3], [4,5,6]])
        b = Numeric.array([7, 8, 9])
        assert_eq(Numeric.cross_product(a,b), [[-6,12,-6],[-3,6,-3]])
        a = Numeric.array([[1,2,3], [4,5,6]])
        b = Numeric.array([[10,11,12], [7,8,9]])
        assert_eq(Numeric.cross_product(a,b,axis1=0,axis2=0), [-33,-39,-45])
        assert_eq(Numeric.cross_product(a,b), [[-9,18,-9], [-3,6,-3]])

    def testChoose (self):
        "Test the choose function."
        assert_eq(Numeric.choose (self.a, (5,4,3,2,1,0)), self.a[::-1])
        assert_eq(Numeric.choose ([[1,0], [0,1]], (66, [(1,2),(11,12)])),
                   [[1,66],[66,12]])
        self.failUnlessRaises(ValueError, Numeric.choose, (0,1,2),([1,1,1],[2,2,2]))

    def testWhere (self):
        "Test the where function."
        assert_eq(Numeric.where((0,1,2), [1,1,1],[2,2,2]), [2,1,1])

    def testSort (self):
        "Test sort, argsort, argmax, argmin, searchsorted"
        s = (3,2,5,1,4,0)
        sm = [s, Numeric.array(s)[::-1]]
        se = Numeric.array(s)[0:0]
        assert_eq(Numeric.sort(s), self.a)
        assert len(Numeric.sort(se)) == 0
        assert_eq(Numeric.argsort(s), [5,3,1,0,4,2])
        assert len(Numeric.argsort(se)) == 0
        assert_eq(Numeric.sort(sm, axis = -1), [[0,1,2,3,4,5],[0,1,2,3,4,5]])
        assert_eq(Numeric.sort(sm, axis = 0), [[0,2,1,1,2,0],[3,4,5,5,4,3]])
        assert_eq(Numeric.searchsorted(Numeric.arange(10), (5,2)), [5,2])
        assert Numeric.argmax(s) == 2
        assert Numeric.argmin(s) == 5
        assert_eq(Numeric.argmax(sm, axis=-1), [2,3])
        assert_eq(Numeric.argmax(sm, axis=1), [2,3])
        assert_eq(Numeric.argmax(sm, axis=0), [0,1,0,1,0,1])
        assert_eq(Numeric.argmin(sm, axis=-1), [5,0])
        assert_eq(Numeric.argmin(sm, axis=1), [5,0])

    def testDiagonal (self):
        "Test the diagonal function."
        b=Numeric.array([[1,2,3,4],[5,6,7,8]]*2)
        assert_eq(Numeric.diagonal(b), [1,6,3,8])
        assert_eq(Numeric.diagonal(b, -1), [5,2,7])
        c = Numeric.array([b,b])
        assert_eq(Numeric.diagonal(c,1), [[2,7,4], [2,7,4]])
        
class UniversalFunctionsTestCase (unittest.TestCase):
    def setUp (self):
        self.a = .01 + Numeric.arange(6) / 8.0 
        self.m = Numeric.array([[1,2,3],[11,12,13]]) / 16.0
      
    def testTrig (self):
        "Test sin, cos, tan, and their inverses"
        assert_eq(Numeric.arccos(-1.0), Numeric.pi)
        assert_eq(Numeric.sin(self.a), map(math.sin, self.a))
        assert_eq(Numeric.cos(self.a), map(math.cos, self.a))
        assert_eq(Numeric.tan(self.a), map(math.tan, self.a))
        assert_eq(Numeric.arccos(self.a), map(math.acos, self.a))
        assert_eq(Numeric.arcsin(self.a), map(math.asin, self.a))
        assert_eq(Numeric.arctan(self.a), map(math.atan, self.a))
        assert Numeric.sin(self.m).shape == self.m.shape
        assert Numeric.cos(self.m).shape == self.m.shape
        assert Numeric.tan(self.m).shape == self.m.shape
        assert Numeric.arcsin(self.m).shape == self.m.shape
        assert Numeric.arccos(self.m).shape == self.m.shape
        assert Numeric.arctan(self.m).shape == self.m.shape
        assert_eq(Numeric.sin(self.m).flat, map(math.sin, self.m.flat))
        assert_eq(Numeric.cos(self.m).flat, map(math.cos, self.m.flat))
        assert_eq(Numeric.tan(self.m).flat, map(math.tan, self.m.flat))
        assert_eq(Numeric.arcsin(self.m).flat, map(math.asin, self.m.flat))
        assert_eq(Numeric.arccos(self.m).flat, map(math.acos, self.m.flat))
        assert_eq(Numeric.arctan(self.m).flat, map(math.atan, self.m.flat))

    def testStandard (self):
        "Test sqrt, log, log10, exp"
        assert_eq(Numeric.exp(1.0), Numeric.e)
        assert_eq(Numeric.sqrt(self.a), map(math.sqrt, self.a))
        assert_eq(Numeric.log(self.a), map(math.log, self.a))
        assert_eq(Numeric.log10(self.a), map(math.log10, self.a))
        assert_eq(Numeric.exp(self.a), map(math.exp, self.a))
        assert_eq(Numeric.sqrt(self.m).flat, map(math.sqrt, self.m.flat))
        assert_eq(Numeric.log(self.m).flat, map(math.log, self.m.flat))
        assert_eq(Numeric.log10(self.m).flat, map(math.log10, self.m.flat))
        assert_eq(Numeric.exp(self.m).flat, map(math.exp, self.m.flat))

    def testLogical (self):
        "Test logical_and, logical_or, sometrue, alltrue"
        x = Numeric.array([1,1,0,0])
        y = Numeric.array([1,0,1,0])
        assert_eq(Numeric.logical_and (x,y), [1,0,0,0])
        assert_eq(Numeric.logical_or (x,y), [1,1,1,0])
        assert Numeric.sometrue(x)
        assert not Numeric.sometrue(Numeric.zeros((3,)))
        assert Numeric.alltrue(Numeric.ones((3,)))
        assert not Numeric.alltrue(x)

class ArrayRelationsToOtherModulesTestCase (unittest.TestCase):
    def testPickle (self):
        "Test pickling of Numeric arrays."
        import pickle
        x = Numeric.arange(10)
        fpik = open('foo.pik', 'wb')
        pickle.dump(x, fpik, 0)
        fpik.close()
        fpik = open('foo.pik', 'rb')
        y = pickle.load(open('foo.pik', 'rb'))
        fpik.close()
        assert_eq(y,[0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
        assert Numeric.alltrue(Numeric.equal(x,y))
        assert Numeric.sometrue(Numeric.equal(x,3))
        assert y.shape == (10,)
        os.remove('foo.pik')

    def copyTest (self):
        "Test how Numeric works with the copy module."
        import copy
        x = Numeric.array([1,2,3])
        y = [1, x, 3]
        c1 = copy.copy(x)
        assert Numeric.allclose(x,c1)
        x[1] = 4
        assert not Numeric.allclose(x,c1)
        c2 = copy.copy(y)
        assert id(c2[1]) == id(Texx)
        c3 = copy.deepcopy(y)
        assert id(c3[1]) != id(x)
        assert Numeric.allclose(c3[1], x)
        
class LinearAlgebraTest (unittest.TestCase):
    def testMatrixMultiply (self):
        "test matrix multiply"
# Oct-23 2002 Greg Smith
# illustrates numpy 'matrixmultiply' bug:
#  (4x4) *  (4x4x2) yields garbage if the
# (4x4x2) matrix is constructed with a transpose
#
        Mat1 = Numeric.array([[ 1.,  0.,  0.,  0.],
               [ 0.,  1.,  0.,  0.],
               [ 0.,  0.,  1.,  0.],
               [ 0.,  0.,  0.,  1.]])
        Mat2 = Numeric.array([[[-2.46970211e+002,  1.06732118e+002],
                [ 1.07496283e+000, -4.64055023e-001],
                [ 2.25803428e-004, -9.74732723e-005],
                [ 1.12163980e-007, -4.83802289e-008]],
               [[ 1.76938633e-001,  1.44333130e+000],
                [-2.68918870e-004,  4.89638391e-004],
                [-9.27555680e-008,  1.18379212e-007],
                [-1.09403911e-010,  8.49856762e-011]],
               [[-4.83160979e-005, -3.92004807e-004],
                [ 6.66762717e-008, -2.27579536e-007],
                [ 2.02732450e-011, -6.23944029e-011],
                [ 6.46933270e-014, -6.92531088e-014]],
               [[ 9.60621511e-009,  7.74154579e-008],
                [-1.26503646e-011,  5.96735496e-011],
                [-1.32003879e-015,  1.64594378e-014],
                [-2.02671071e-017,  2.56964410e-017]]])
        result1 = Numeric.matrixmultiply( Mat1,Mat2 )
#same thing again, except a matrix identical to Mat2 is
# constructed by transposing:
#

        Mat2a = Numeric.transpose(Numeric.array(\
        [[[-2.46970211e+002,  1.76938633e-001, -4.83160979e-005,  9.60621511e-009],
        [ 1.07496283e+000, -2.68918870e-004,  6.66762717e-008, -1.26503646e-011],
        [ 2.25803428e-004, -9.27555680e-008,  2.02732450e-011, -1.32003879e-015],
        [ 1.12163980e-007, -1.09403911e-010,  6.46933270e-014, -2.02671071e-017]],
       [[ 1.06732118e+002,  1.44333130e+000, -3.92004807e-004,  7.74154579e-008],
        [-4.64055023e-001,  4.89638391e-004, -2.27579536e-007,  5.96735496e-011],
        [-9.74732723e-005,  1.18379212e-007, -6.23944029e-011,  1.64594378e-014],
        [-4.83802289e-008,  8.49856762e-011, -6.92531088e-014,  2.56964410e-017]]]))

        result2 = Numeric.matrixmultiply( Mat1,Mat2a )
        self.failUnless(eq(result1, result2))

    def testgeneralizedInverse (self):
        "Test LinearAlgebra.generalized_inverse"
        import LinearAlgebra
        ca = Numeric.array([[1,1-1j],[0,1]])
        cai = LinearAlgebra.inverse(ca)
        cai2 = LinearAlgebra.generalized_inverse(ca)
        self.failUnless(eq(cai, cai2))

    def testSVD(self):
        """
        From bug #930735. Numbers redone in Maple.
        """
        import LinearAlgebra
        a = Numeric.array([[2,4],[1,3],[0,0],[0,0]])
        u, s, vt = LinearAlgebra.singular_value_decomposition(a)
        s34d2 = math.sqrt(34.)/2
        s26d2 = math.sqrt(26.)/2
        assert_eq(s, Numeric.array([s34d2+s26d2, s34d2-s26d2]))
        vt_c = Numeric.array([[-0.404553584833756919, -0.914514295677304467],
                              [-0.914514295677304467,  0.404553584833756919]])
        assert_eq(vt, vt_c)
        u_c = Numeric.array([[-0.817415560470363233, -0.576048436766320782],
                             [-0.576048436766320782,  0.817415560470363344],
                             [0, 0],
                             [0, 0]])
        assert_eq(u, u_c)
        assert_eq(a, Numeric.dot(u*s, vt))

    def testLinearLeastSquares(self):
        """
        From bug #503733.
        """
        # XXX not positive on this yet
        import LinearAlgebra
        from RandomArray import seed, random
        seed(7,19)
        (n, m) = (180, 35)
        yp = random((n,m))
        y  = random(n)
        x, residuals, rank, sv = LinearAlgebra.linear_least_squares(yp, y)
        # shouldn't segfault.
        assert rank == m

    def testLinearLeastSquares2(self):
        """
        From bug #503733. Failing with dlapack_lite
        """
        import LinearAlgebra
        d = [0.49910197] + [0.998203938] * 49
        d1 = [0.000898030454] * 50
        def tridiagonal(sz):
            G = Numeric.zeros((sz,sz), Numeric.Float64)
            for i in range(sz):
                G[i,i] = d[i]
            for i in range(0,sz-1):
                G[i+1,i] = G[i,i+1] = d1[i]
            return G

        yfull = Numeric.array(
            [4.37016668e+18, 4.09591905e+18, 3.82167167e+18, 4.12952660e+18,
             2.60084719e+18, 2.05944452e+18, 1.69850960e+18, 1.51450383e+18,
             1.39419275e+18, 1.25264986e+18, 1.18187857e+18, 1.16772440e+18,
             1.17126300e+18, 1.13941580e+18, 1.17834000e+18, 1.20664860e+18,
             1.18895580e+18, 1.18895580e+18, 1.21726440e+18, 1.24557296e+18,
             1.22434149e+18, 1.23495719e+18, 1.24203436e+18, 1.22434160e+18,
             1.23495720e+18, 1.21372580e+18, 1.22434160e+18, 1.21018740e+18,
             1.22080300e+18, 1.15357020e+18, 1.19957160e+18, 1.18187880e+18,
             1.19249440e+18, 1.18895579e+18, 1.28449704e+18, 1.27742021e+18,
             1.30218984e+18, 1.30926700e+18, 1.25972716e+18, 1.15003156e+18,
             1.17126296e+18, 1.15710876e+18, 1.10756882e+18, 1.20311006e+18,
             1.29511286e+18, 1.28449726e+18, 1.29157446e+18, 1.44373273e+18,])
        for i in range(20, 40):
            G = tridiagonal(i)
            y = yfull[:i]
            A = LinearAlgebra.linear_least_squares(G, y)[0]
            total = Numeric.add.reduce(y)
            residual = Numeric.absolute(y - Numeric.dot(G, A))
            assert_eq(0.0, Numeric.add.reduce(residual)/total)

    def testEigenvalues(self):
        """
        From bug #545259 -- illegal value to DGEEV routine
        """
        import LinearAlgebra
        a = Numeric.array([[2,3], [4,5]])
        v = LinearAlgebra.eigenvalues(a)
        assert_eq(v, Numeric.array([3.5-math.sqrt(57)/2.,
                                    3.5+math.sqrt(57)/2.]))

    def testHeigenvalues(self):
        """
        From bug #464980.
        """
        import LinearAlgebra
        a = Numeric.array([[1.0, 0.01j], [-0.01j, 1.0]])
        v = LinearAlgebra.eigenvalues(a)
        assert_eq(v, [1.01+0.j, 0.99+0.j])
        Hv = LinearAlgebra.Heigenvalues(a)
        assert_eq(v, [1.01+0.j, 0.99+0.j])

if __name__ == '__main__':
    unittest.main()
