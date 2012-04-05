desc = "testing cgi input of doubles"

d = 1.25
f = 3.75

filedata = "${d} ${f}"
script_path = "double?file=$[0]&d=%f&f=%f" % (d,f)
outcome = "%0.2f %0.2f" % (d, f)
