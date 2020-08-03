
import timeit

setup = u"""
import mrhttp, cgi
x = 406785442332432
x64 = mrhttp.to64(x)
txt = u"This is going to be escaped <yay> 你们好"
"""

print ("  ",(min(timeit.Timer('mrhttp.to64(x)', setup=setup).repeat(100000, 3))))
print ("  ",(min(timeit.Timer('mrhttp.from64(x64)', setup=setup).repeat(100000, 3))))
print ("  ",(min(timeit.Timer('mrhttp.escape(txt)', setup=setup).repeat(100000, 3))))
print ("  ",(min(timeit.Timer('cgi.escape(txt)', setup=setup).repeat(100000, 3))))

