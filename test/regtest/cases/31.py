desc =  "test casts of uints, ints, etc (27.py from OKWS 2.1)"

def convert (x):
    if (x < 0): return "%d" % x
    else: return "0x%x" % x

def json_print (i):
    return "%d" % i

dct = { "u1" :  0x1000,
        "u2" :  0x89abcdef01234567,
        "u3" :  0xffff0000ffff0000,
        "u4" :  0xffffffffffffffff,
        "i1" :  0x7777000077770000 * -1,
        "i2" :  0xf00000 * -1 }

filedata = '\n'.join (["${%s}" % k for k in dct.keys () ] )

vars = '&'.join (["vars=%s" % k for k in dct.keys () ])
vals = '&'.join (["%s=%s" % (k, convert (dct[k])) for k in dct.keys () ])

script_path = "reflect?file=$[0]&%s&%s" % (vars, vals)

outcome='\n'.join ([ json_print (dct[k]) for k in dct.keys () ])
