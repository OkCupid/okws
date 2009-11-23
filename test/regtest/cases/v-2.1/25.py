
desc = "test XML/RPC Pub interface"

n = 10
m = 5

v  = [ { "val" : range (i, i+m) }  for i in  range (0, n) ]

##-----------------------------------------------------------------------

filedata = """
{% for (o, objs) {{
    %{o.val} %{o.iter}
}}
%}
"""

##-----------------------------------------------------------------------

def _fetch (case):
    import xmlrpclib
    import sys

    url = case.config ().url ("xmlrpc")
    server = xmlrpclib.Server (url)

    args = { "objs" : v }

    res = server.okws.pub3 ( { "filename" : case.include_path (0),
                               "options" : ( "visibleErrors",
                                             "verbose") ,
                               "variables" : args })
    return str (res['data'])


custom_fetch = _fetch

##-----------------------------------------------------------------------

outcome = '\n'.join ([ "%s %d" % (v[i]["val"], i)
                       for i in range (0, len (v)) ])
