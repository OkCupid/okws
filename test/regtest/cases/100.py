import json
import urllib

desc = "test of cgi_encode"

orig = " 1234567890-=~!@#$%^&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZXCVBNM<>?".lower ()

enc = urllib.quote_plus (orig).lower()

filedata = """
{$
   print (url_escape ("%(orig)s"));
$} 

""" % { "orig" : orig }

outcome = enc
