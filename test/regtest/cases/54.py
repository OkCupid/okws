#
# a test for more complicated vector operations
#

vals = [ i*10+3 for i in range (0,30)]
indices = "range(10,28,3)"

d = { "v" : [ { "val" : i } for i in vals ] }

filedata =  """
{$ locals %s $}
{$ for (i, %s) {{ ${v[i].val} }} $}
""" % (d, indices)

outcome = " ".join (["%d" % vals[i] for i in eval (indices) ])

desc = "a slightly more complicated vector/index example"

