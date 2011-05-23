description = "check the bitwise_foo filters"

filedata = """
{$ 
   locals { out : [] }
   append (out, bitwise_and (1023,511,255,127,63,31,15));
   append (out, bitwise_xor (0xabcdeffa, 0x183819234));
   append (out, bitwise_leftshift (0x3, 0x10));
   append (out, bitwise_rightshift (0xa44b42adee, 0x10));
   print (join (" ", out));
$}"""

vals = [  1023 & 511 & 255 & 127 & 63 & 31 & 15,
          0xabcdeffa ^ 0x183819234, 
          0x3 <<  0x10,
          0xa44b42adee >> 0x10 ]

outcome = " ".join (str (x) for x in vals)
