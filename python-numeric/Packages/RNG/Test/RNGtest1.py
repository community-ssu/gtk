from Numeric import *
import RNG

def test_normal (mean, std, n=10000):
    dist = RNG.NormalDistribution(mean, std)
    rng = RNG.CreateGenerator(0, dist)
    values = rng.sample(n)
    m = sum(values)/n
    s = sqrt(sum((values-m)**2)/n)
    return (m, s)

def test_lognormal (mean, std, n=10000):
    dist = RNG.LogNormalDistribution(mean, std)
    rng = RNG.CreateGenerator(0, dist)
    values = rng.sample(n)
    m = sum(values)/n
    s = sqrt(sum((values-m)**2)/n)
    return (m, s)

def test_uniform (n=10000):
    dist = RNG.UniformDistribution(0., 1.)
    rng = RNG.CreateGenerator(0, dist)
    values = rng.sample(n)
    m = sum(values)/n
    return m

if __name__ == "__main__":
    while(1):
       n = input("Enter sample size, 0 to quit: ")
       if (n <= 1): raise SystemExit
       print "Should be close to 1/2:", test_uniform(n)
       print "Should be close to 10.0, 1.0:", test_normal(10.0, 1.0,n)
       print "Should be close to 10.0, 1.0:", test_lognormal(10.0, 1.0,n)
