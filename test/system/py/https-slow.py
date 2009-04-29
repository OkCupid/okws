
#
# A little program for testing what happens to the HTTPS server when up
# against a slow client just dribbling out bytes.
#
import httplib
import time
import socket

x = httplib.HTTPSConnection ("localhost", 4430)

try:
    for i in range (0,20):
        time.sleep (1)
        x.send ("x")
except socket.error:
    print "Good, socket closed!"
