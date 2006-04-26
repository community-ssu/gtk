# The Statistics module is not part of the RNG package. It is
# distributed by Konrad Hinsen on the Starship.
# See http://starship.skyport.net/crew/hinsen
import Numeric
import RNG
dist = RNG.LogNormalDistribution(10.,5.)
rng = RNG.CreateGenerator(0,dist)
values = rng.sample(50000)
print "generated values"

from RNG import Statistics
upper=40.
lower=1.
numbins=100
hist = Statistics.histogram(values,numbins,(lower,upper))
print "generated histogram"
print hist

for i in range(numbins):
  print i, hist[i,0], hist[i,1], 50000*((upper-lower)/numbins)*dist.density(hist[i,0]), hist[i,1]/(50000*((upper-lower)/numbins)*dist.density(hist[i,0]))


