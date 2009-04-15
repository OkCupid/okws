
rows = [ { "val_i" : i, 
           "str_i" : "str-%d-%d-str" %  (i, i) } 
         for i in range (0,20) ]

filedata = [
"""
{$ setl { rows : %s } $}
{$ for (r, rows) {
    include ("$[1]", { row : r })
} $}

""" % rows,
"""
{$ eval (import (row));  $}
${iter} ${val_i} ${str_i}
"""]

desc = "test the import function"

outcome = '\n'.join (["%d %d %s" % (r["val_i"], r["val_i"], r["str_i"]) 
                      for r in rows ])
