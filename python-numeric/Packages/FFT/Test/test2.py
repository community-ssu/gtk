from FFT import *

def test():
    print fft( (0,1)*4 )
    print inverse_fft( fft((0,1)*4) )
    print fft( (0,1)*4, n=16 )
    print fft( (0,1)*4, n=4 )

    print fft2d( [(0,1),(1,0)] )
    print inverse_fft2d (fft2d( [(0, 1), (1, 0)] ) )
    print real_fft2d([(0,1),(1,0)] )
    print real_fft2d([(1,1),(1,1)] )


    import sys

    oosq2 = 1.0/Numeric.sqrt(2.0)

    toler = 1.e-10

    p = Numeric.array(((1, 1, 1, 1, 1, 1, 1, 1),
                       (1, oosq2, 0, -oosq2, -1, -oosq2, 0, oosq2),
                       (1, 0, -1, 0, 1, 0, -1, 0),
                       (1, -oosq2, 0, oosq2, -1, oosq2, 0, -oosq2),
                       (1, -1, 1, -1, 1, -1, 1, -1),
                       (1, 0, 0, 0, 0, 0, 0, 0),
                       (0, 0, 0, 0, 0, 0, 0, 0)))

    q = Numeric.array(((8,0,0,0,0,0,0,0),
                       (0,4,0,0,0,0,0,4),
                       (0,0,4,0,0,0,4,0),
                       (0,0,0,4,0,4,0,0),
                       (0,0,0,0,8,0,0,0),
                       (1,1,1,1,1,1,1,1),
                       (0,0,0,0,0,0,0,0)))

    def cndns(m):
        return Numeric.maximum.reduce(abs(m).flat)

    try:
        junk = hermite_fft
        new = 1
    except NameError:
        new = 0

    # Tests for correctness.
    # Somewhat limited since
    # p (and thus q also) is real and hermite, and the dimension we're
    # testing is a power of 2.  If someone can cook up more general data
    # in their head or with another program/library, splice it in!

    print "\nCorrectness testing dimension -1."

    sys.stdout.write("fft: ")
    sys.stdout.flush()
    P = fft(p)
    if cndns(P-q) / cndns(q) > toler:
        print "inaccurate"
    else:
        print "OK"
        
    sys.stdout.write("real_fft: ")
    sys.stdout.flush()
    RP = real_fft(p)
    npt = p.shape[-1]
    rpt = npt/2 + 1
    qr = q[:,:rpt]
    if cndns(RP-qr) / cndns(qr) > toler:
        print "inaccurate"
    else:
        print "OK"


    sys.stdout.write("inverse_real_fft: ")
    sys.stdout.flush()
    if cndns(inverse_real_fft(q, npt)-p) / cndns(p) > toler:
        print "inaccurate"
    else:
        print "OK"

    # now just test consistency

    for dim in range(len(p.shape)):
        print "\nConsistency testing dimension %d, length %d." % \
              (dim, p.shape[dim])

        sys.stdout.write("fft/inverse_fft: ")
        sys.stdout.flush()
        P = fft(p, None, dim)
        Q = inverse_fft(P, None, dim)
        if cndns(Q-p) / cndns(p) > toler:
            print "inconsistent"
        else:
            print "OK"

        sys.stdout.write("fft/real_fft: ")
        sys.stdout.flush()
        RP = real_fft(p, None, dim)
        npt = p.shape[dim]
        rpt = npt/2 + 1

        P = Numeric.take(P, range(rpt), dim)
        if cndns(RP-P) / cndns(RP) > toler:
            print "inconsistent"
        else:
            print "OK"

        sys.stdout.write("inverse_fft/inverse_hermite_fft: ")
        sys.stdout.flush()
        hp = inverse_hermite_fft(q, npt, dim)
        Q = inverse_fft(q, None, dim)
        Q = Numeric.take(Q, range(rpt), dim)
        if cndns(hp-Q) / cndns(hp) > toler:
            print "inconsistent"
        else:
            print "OK"
            
        sys.stdout.write("real_fft/inverse_real_fft: ")
        sys.stdout.flush()
        if cndns(inverse_real_fft(RP, npt, dim)-p) / cndns(p) > toler:
            print "inconsistent"
        else:
            print "OK"

        sys.stdout.write("inverse_hermite_fft/hermite_fft: ")
        sys.stdout.flush()
        if cndns(hermite_fft(hp, npt, dim)-q) / cndns(q) > toler:
            print "inconsistent"
        else:
            print "OK"

        print ""
        
    # test multi-dim stuff
    print "Multi-dimensional tests:"

    tee = Numeric.array(((2.0, 0, 2, 0),
                         (0, 2, 0, 2),
                         (2, 0, 2, 0),
                         (0, 2, 0, 2)))
    eff = Numeric.array(((16.0, 0, 0, 0),
                         (0, 0, 0, 0),
                         (0, 0, 16, 0),
                         (0, 0, 0, 0)))

    sys.stdout.write("fftnd: ")
    ftest = fftnd(tee)
    if cndns(ftest - eff) / cndns(eff) > toler:
        "inaccurate"
    else:
        print "OK"

    sys.stdout.write("inverse_fftnd: ")
    if cndns(inverse_fftnd(ftest) - tee) / cndns(tee) > toler:
        print "inconsistent with fftnd"
    else:
        print "OK"

    sys.stdout.write("real_fftnd: ")
    fred = real_fftnd(p)
    npts = p.shape[-1]
    rpts = npts/2 + 1
    actual = fftnd(p)
    ract = actual[..., :rpts]
    if cndns(fred-ract) / cndns(ract) > toler:
        print "inconsistent with fftnd"
    else:
        print "OK"
        
    sys.stdout.write("inverse_real_fftnd: ")
    ethel = inverse_real_fftnd(fred)
    if cndns(p-ethel) / cndns(p) > toler:
        print "inconsistent with real_fftnd"
        print fred
        print ethel
        print p
    else:
        print "OK"

    sys.stdout.write("\nfft2d shape test: ")
    success = 1
    axes = (0,1)
    shape = (7,4)
    barney = Numeric.zeros(shape,'d')
    betty = fft2d(barney)
    success = success and betty.shape == barney.shape
    betty = fft2d(barney, None, axes)
    success = success and betty.shape == barney.shape
    betty = fft2d(barney, shape)
    success = success and betty.shape == barney.shape
    betty = fft2d(barney, shape, axes)
    success = success and betty.shape == barney.shape
    betty = real_fft2d(barney)
    wilma = inverse_real_fft2d(betty)
    success = success and wilma.shape == barney.shape
    wilma = inverse_real_fft2d(betty, shape)
    success = success and wilma.shape == barney.shape
    wilma = inverse_real_fft2d(betty, None, axes)
    success = success and wilma.shape == barney.shape
    wilma = inverse_real_fft2d(betty, shape, axes)
    success = success and wilma.shape == barney.shape
    if success:
        print "OK"
    else:
        print "fail"

    sys.stdout.write("\nCodelet order test: ")
    sys.stdout.flush()
    from RandomArray import random
    success = 1
    for size in range (1,25): 
        for i in range(3): 
            a = random(size) 
            b = real_fft(a) 
            c = inverse_real_fft(b,size) 
            if cndns(c-a) / cndns(a) > toler:
                print "real transforms failed for size %d" % size
                success = 0
            a = a + random(size) * 1j
            b = fft(a) 
            c = inverse_fft(b,size) 
            if cndns(c-a) / cndns(a) > toler:
                print "complex transforms failed for size %d" % size
                success = 0
    if success:
        print "OK"

if __name__ == '__main__':
    test()
