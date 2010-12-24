import json
import urllib
import base64

#
# A **generic** test case that implements its own run() script
#

desc = "test of json2xdropq and xdropq2json; also tests a lot of the xdr2json stack, while we're at it."

def fetch (tc, args):
    """A shortcut for fetching from the encoder service running on OKWS."""
    url = tc._config.url ("encoder")
    query = urllib.urlencode ( args )
    resp = ''.join (urllib.urlopen (url + "?" + query))
    obj = json.loads (resp)
    return obj["data"]

def run (tc, codes):
    """The hook called by the test case runner..."""

    # don't run this case on local data mode
    if tc.is_local ():
        return codes.SKIPPED

    arg = { 
        "procs" : { 
            "typ" : 1,
            "procs" : [
                { "name" : "blah",
                  "brother_id" : 2,
                  "num_brothers" : 4 },
                { "name" : "blah-blah",
                  "brother_id" : 3,
                  "num_brothers" : 4 }
                ]
            },
        "data" : {
            "data" : base64.b64encode ("\x03\x05ABC\x55\x00GGHH")
            }
        }
    
    earg = base64.b64encode (json.dumps (arg))
    typ = "ok_custom_arg_t"
    args1 = { "encode" : earg, "type" : typ } 
    resp1 = fetch (tc, args1)
    args2 = { "decode" : resp1, "type" : typ }
    resp2 = fetch (tc, args2)
    
    if resp2 == arg:
        tc.report_success ()
        ret = codes.OK
    else:
        tc.report_failure ("expected %s ; got %s" % (arg, resp2))
        ret = codes.FAILED
    return ret
