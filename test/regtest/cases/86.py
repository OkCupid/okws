description = "test regex/replace"

filedata = """{%
    locals { s : "Jam-aA--pickle--AaAa---pockle-aaAaaa-blah-Aaa-blah--" }
    print (replace (s, r/-+[Aa]+-+/, " "));
%}"""

outcome = "Jam pickle pockle blah blah--" 
