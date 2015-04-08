"""
    pubast
    ~~~~~~

    The `pubast` module provides a python representation of pub's abstract
    syntax trees.

    :copyright: 2014 Humor rainbow, 2008 by Armin Ronacher
    :licence: BSD
"""
# This file contains code taken from Python's ast.py with the additional
# copyright:  Copyright 2008 by Armin Ronacher.

from okws._config import BINDIR
from okws._pubast import mk_file, Node, Metadata
from enum import Enum
import subprocess
import base64
import pprint
import json

_PARSER = BINDIR + "/pub3astdumper"

# check_output is not available in 2.6...
if "check_output" not in dir( subprocess ):
    def __pubast_checkoutput(*popenargs, **kwargs):
        if 'stdout' in kwargs:
            raise ValueError('stdout argument not allowed, it will be overridden.')
        process = subprocess.Popen(stdout=subprocess.PIPE, *popenargs, **kwargs)
        output, unused_err = process.communicate()
        retcode = process.poll()
        if retcode:
            cmd = kwargs.get("args")
            if cmd is None:
                cmd = popenargs[0]
            raise subprocess.CalledProcessError(retcode, cmd)
        return output
else:
    __pubast_checkoutput = subprocess.check_output


def parse(file):
    """Parse the given file as a pub file and returns the ast."""
    res = json.loads(__pubast_checkoutput([_PARSER, "--", file]))
    return mk_file(res)


def parse_string(s):
    """Parse the given string as a pub file and returns the ast."""
    ps = subprocess.Popen([_PARSER],
                          stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE)
    raw_ast, stderr = ps.communicate(s)
    if ps.returncode:
        raise subprocess.CalledProcessError(ps.returncode, _PARSER, stderr)
    res = json.loads(raw_ast)
    return mk_file(res)


def iter_fields(node):
    """
    Yield a tuple of ``(fieldname, value)`` for each field in ``node._fields``
    that is present on *node*.
    """
    for field in node._fields:
        try:
            yield field, getattr(node, field)
        except AttributeError:
            pass


def iter_child_nodes(node):
    """
    Yield all direct child nodes of *node*, that is, all fields that are nodes
    and all items of fields that are lists of nodes.
    """
    for name, field in iter_fields(node):
        if isinstance(field, Node):
            yield field
        elif isinstance(field, list):
            for item in field:
                if isinstance(item, Node):
                    yield item


def walk(node):
    """
    Recursively yield all descendant nodes in the tree starting at *node*
    (including *node* itself), in no specified order.  This is useful if you
    only want to modify nodes in place and don't care about the context.
    """
    from collections import deque
    todo = deque([node])
    while todo:
        node = todo.popleft()
        todo.extend(iter_child_nodes(node))
        yield node


class NodeVisitor(object):
    """
    This class is a copy of python's AST.py visitor except it walks up the node
    class hierarchy when trying to find a visitor (e.g.: for an `ExprLambda`
    we'd try to call `visit_ExprLambda` and then `visit_Expr`).

    Python's doc:

    A node visitor base class that walks the abstract syntax tree and calls a
    visitor function for every node found.  This function may return a value
    which is forwarded by the `visit` method.

    This class is meant to be subclassed, with the subclass adding visitor
    methods.

    Per default the visitor functions for the nodes are ``'visit_'`` +
    class name of the node.  So a `TryFinally` node visit function would
    be `visit_TryFinally`.  This behavior can be changed by overriding
    the `visit` method.  If no visitor function exists for a node
    (return value `None`) the `generic_visit` visitor is used instead.

    Don't use the `NodeVisitor` if you want to apply changes to nodes during
    traversing.  For this a special visitor exists (`NodeTransformer`) that
    allows modifications.
    """

    def visit(self, node):
        """Visit a node."""
        cls = node.__class__
        visitor = None
        while cls != Node and visitor is None:
            method = 'visit_' + cls.__name__
            visitor = getattr(self, method, None)
            # Pop up one level in the hierarchy
            if visitor is None:
                new_cls = None
                for sup in cls.__bases__:
                    # We assume we have only parent that inherits from Node
                    # no diamond inheritance here.
                    if issubclass(sup, Node):
                        new_cls = sup
                        break
                assert new_cls is not None
                cls = new_cls
        if visitor is None:
            visitor = self.generic_visit
        self._path.append(node)
        if self._fname is None and isinstance(node, Metadata):
            self._fname = node.jailed_filename
        res = visitor(node)
        self._path.pop()
        return res
        return visitor(node)

    def generic_visit(self, node):
        """Called if no explicit visitor function exists for a node."""
        for field, value in iter_fields(node):
            if isinstance(value, list):
                for item in value:
                    if isinstance(item, Node):
                        self.visit(item)
            elif isinstance(value, Node):
                self.visit(value)

    def __init__(self, fname=None):
        self._fname = fname
        self._path = []

    def parent(self):
        if len(self._path) > 1:
            return self._path[-2]
        else:
            return None

    def filename(self):
        return self._fname

    def lineno(self):
        for n in reversed(self._path):
            if hasattr(n, "lineno"):
                return n.lineno

    def pos(self):
        fname = self._fname if self._fname is not None else "<unknown>"
        lineno = self.lineno()
        if lineno is None:
            lineno = 0
        return "%s:%i" % (self._fname, lineno)


class NodeTransformer(NodeVisitor):
    """
    A :class:`NodeVisitor` subclass that walks the abstract syntax tree and
    allows modification of nodes.

    The `NodeTransformer` will walk the AST and use the return value of the
    visitor methods to replace or remove the old node.  If the return value of
    the visitor method is ``None``, the node will be removed from its location,
    otherwise it is replaced with the return value.  The return value may be the
    original node in which case no replacement takes place.

    Here is an example transformer that rewrites all occurrences of name lookups
    (``foo``) to ``data['foo']``::

       class RewriteName(NodeTransformer):

           def visit_Name(self, node):
               return copy_location(Subscript(
                   value=Name(id='data', ctx=Load()),
                   slice=Index(value=Str(s=node.id)),
                   ctx=node.ctx
               ), node)

    Keep in mind that if the node you're operating on has child nodes you must
    either transform the child nodes yourself or call the :meth:`generic_visit`
    method for the node first.

    For nodes that were part of a collection of statements (that applies to all
    statement nodes), the visitor may also return a list of nodes rather than
    just a single node.

    Usually you use the transformer like this::

       node = YourTransformer().visit(node)
    """

    def generic_visit(self, node):
        for field, old_value in iter_fields(node):
            old_value = getattr(node, field, None)
            if isinstance(old_value, list):
                new_values = []
                for value in old_value:
                    if isinstance(value, Node):
                        value = self.visit(value)
                        if value is None:
                            continue
                        elif not isinstance(value, Node):
                            new_values.extend(value)
                            continue
                    new_values.append(value)
                old_value[:] = new_values
            elif isinstance(old_value, Node):
                new_node = self.visit(old_value)
                if new_node is None:
                    delattr(node, field)
                else:
                    setattr(node, field, new_node)
        return node


class _Dumper:
    """Internal class used to help dumping AST nodes"""
    HIDE_ATTR = ["lineno"]

    def up(self, maxdepth):
        return maxdepth - 1 if maxdepth is not None else None

    def dump(self, node, maxdepth):
        if maxdepth is not None and maxdepth <= 0:
            return "..."
        subdepth = self.up(maxdepth)
        if isinstance(node, list):
            return [self.dump(x, subdepth) for x in node]
        if not isinstance(node, Node):
            return node
        method = 'dump_' + node.__class__.__name__
        f = getattr(self, method, self.generic_dump)
        return f(node, subdepth)

    def dump_Zstr(self, node, maxdepth):
        return base64.b64decode(node.s)

    def dump_ZoneText(self, node, maxdepth):
        return ("ZoneText", self.dump(node.original_text, self.up(maxdepth)))

    def dump_File(self, node, maxdepth):
        return self.dump(node.root.zones, maxdepth)

    def generic_dump(self, node, maxdepth):
        if isinstance(node, Enum):
            return node.name
        fields = {}
        subdepth = self.up(maxdepth)
        for k, v in iter_fields(node):
            if v is not None and k not in self.HIDE_ATTR:
                fields[k] = self.dump(v, subdepth)
        name = node.__class__.__name__
        if len(fields) == 1:
            return (name, fields.values()[0])
        elif len(fields) == 0:
            return name
        else:
            return (name, fields)


def dump(node, maxdepth=None):
    """Prints out a node to stdout"""
    return pprint.pprint(_Dumper().dump(node, maxdepth))


## Simple example:

class _LambdaPrinter(NodeVisitor):
    """ A very simple visitor example that prints all the lambdas it
    encounters with a specific printer and all the other expr with a generic
    printer"""
    def visit_ExprLambda(self, node):
        print "lambda(%s)" % ", ".join(node.params)
        self.generic_visit(node)

    def visit_Expr(self, node):
        print node.__class__.__name__
        self.generic_visit(node)

if __name__ == "__main__":
    import sys
    fname = sys.argv[1]
    v = parse(fname)
    _LambdaPrinter().visit(v)
