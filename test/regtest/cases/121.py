desc = "testing remove (from dictionary)"

filedata = \
"""{$
    locals { dict : {a:1,b:2,c:3} }
    print(remove(dict,"a"));
    print(dict);
    print(remove(dict,"d"));
    print(dict);
$}"""

outcome = """true{"c" : 3, "b" : 2}false{"c" : 3, "b" : 2}"""
