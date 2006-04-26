from Numeric import * 
from RandomArray import random 
tolerance = .001 
from FFT import real_fft,inverse_real_fft 

for size in range (1,25): 
    for i in range(3): 
        a = random(size) 
        b = real_fft(a) 
        c = inverse_real_fft(b,size) 
        assert not sometrue(greater(abs(a-c),tolerance))

  #example from previously fixed real_fft bug (OK) 
x = cos(arange(30.0)/30.0*2*pi) 
y = real_fft(x) 
z = inverse_real_fft(y,30) 
assert not sometrue(greater(abs(x-z),tolerance)) 
