from Numeric import dot,sum 
import sys,numeric_version 
import RandomArray 
import LinearAlgebra 

print sys.version 
print "Numeric version:",numeric_version.version 

RandomArray.seed(123,456) 
a = RandomArray.normal(0,1,(100,10)) 
f = RandomArray.normal(0,1,(10,30)) 
e = RandomArray.normal(0,0.1,(100,30)) 
print "Got to seed:",RandomArray.get_seed() 

b = dot(a,f)+e 

(x,res,rank,s)=LinearAlgebra.linear_least_squares(a,b) 

f_res = sum((b-dot(a,f))**2) 
x_res = sum((b-dot(a,x))**2) 

print "'Planted' residues, upper bound for optimal residues:" 
print f_res 
print "Claimed residues:" 
print res 
print "Actual residues:" 
print x_res 
print "Ratio between actual and claimed (shoudl be 1):" 
print x_res/res 
print "Ratio between actual and planted (should be <1):" 
print x_res/f_res 
print "Ratio between claimed and planted (shoudl be <1):" 
print res/f_res 
print 

