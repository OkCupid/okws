import json
import urllib

desc = "test of cgi_encode"

orig = "`1234567890-=~!@#$%^&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZZXCVBNM<>?"

enc = urllib.encode (orig)

filedata = """
{$
   print (url_encode (%{orig}s));
$} 

""" % { "orig" : orig }

outcome = enc
