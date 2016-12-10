"""
    rpcast
    ~~~~~~

    The `rpcast` module provides a python representation of prot's abstract
    syntax trees.

    :licence: BSD
"""
# This file contains code taken from Python's ast.py with the additional
# copyright:  Copyright 2008 by Armin Ronacher.

from okws._config import BINDIR
from okws._common import okws_checkoutput
import json
import pprint

_PARSER = BINDIR + "/xmlrpcc"

def parse(file):
    """Parse the given file as a prot file and returns the ast."""
    res = json.loads(okws_checkoutput([_PARSER, '-a', file, '-o', '-']))
    return res

def is_typedecl(node):
    """Returns true if this line creates a new C++ type (either as a typedef, a
    struct, a union..."""
    return node["node_type"] in ["decl", "union", "struct", "enum"]

def get_used_types(node):
    """Returns the set of RPC types referenced in a given declaration"""
    if isinstance (node, list):
        res=set()
        for d in node:
            res |= get_used_types(d)
        return res
    ty=node["node_type"]
    if ty == "decl":
        return set([node["type"]])
    elif ty == "const":
        return set()
    elif ty == "literal":
        return set()
    elif ty == "struct":
        return get_used_types(node["decls"])
    elif ty == "enum":
        return set()
    elif ty == "utag":
        return get_used_types(node["tag"])
    elif ty == "union":
        v = get_used_types(node["cases"])
        v.add(node["tagtype"])
        return v
    elif ty == "arg":
        return set([node["type"]])
    elif ty == "proc":
        return get_used_types(node["arg"]) | get_used_types(node["res"])
    elif ty == "vers":
        return get_used_types(node["procs"])
    elif ty == "program":
        return get_used_types(node["vers"])
    elif ty == "namespace":
        return get_used_types(node["progs"])
    else:
        raise Error("Unknown node_type %s" % ty)

if __name__ == "__main__":
    import sys
    fname = sys.argv[1]
    v = parse(fname)
    # for x in v:
    #     print get_used_types(x)
    pprint.pprint(v)
