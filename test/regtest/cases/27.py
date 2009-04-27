
desc =  "test casts of uints, ints, etc"

dct = { "u1" :  0x1000,
        "u2" :  0x89abcdef01234567,
        "u3" :  0xffff0000ffff0000,
        "u4" :  0xffffffffffffffff,
        "i1" :  0x7777000077770000 * -1  }

filedata = '\n'.join (["${%s}" % k for k in dct.keys () ] )

vars = '&'.join (["vars=%s" % k for k in dct.keys () ])
vals = '&'.join (["%s=%s" % (k, dct[k]) for k in dct.keys () ])

script_path = "reflect?file=$[0]&%s&%s" % (vars, vals)

outcome='\n'.join ([ "%d" % dct[k] for k in dct.keys () ])
