
##-----------------------------------------------------------------------

rows = [ { "val_i" : i, 
           "str_i" : "str-%d-%d-str" %  (i, i) } 
         for i in range (0,20) ]

importline = "eval (import (row))"
importline_pub = "{$ %s $}" % importline

parent ="""
{$ setl { rows : %s } $}
{$ for (row, rows) {
    %s
    include ("$[1]", { row : row })
} $}

"""

child = "%s\n${iter} ${val_i} ${str_i}"

##-----------------------------------------------------------------------

outcome = '\n'.join (["%d %d %s" % (r["val_i"], r["val_i"], r["str_i"]) 
                      for r in rows ])

##-----------------------------------------------------------------------

cases = [ { "filedata" : [ parent % (rows, ""), child % importline_pub ],
            "desc" : "import function, nesting A",
            "outcome" : outcome },
          { "filedata" : [ parent % (rows, importline), child % "" ],
            "desc" : "import function, nesting B",
            "outcome" : outcome } ]
          
