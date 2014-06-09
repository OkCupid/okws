import re
import sys
import os

###############################################################################
# lexer
###############################################################################

# opaque? unsigned? void?
_keywords = frozenset(["typedef", "namespace", "void", "unsigned", "enum",
                       "struct", "union", "switch", "case", "default"])
_tokens = ["identifier",
           "int",
           "obrack",
           "cbrack",
           "semi",
           "ocurl",
           "ccurl",
           "eq",
           "comma",
           "carrets",
           "star",
           "opar",
           "cpar",
           "colon",
           "eof"] \
    + list(_keywords)


_int_types = frozenset(["int", "hyper", "uint_64_t", "u_int32_t"])
_native_types = _int_types | frozenset(["string", "void", "opaque", "bool"])


class _TokHelper:
    """A helper class that emulates an enum for all the tokens in the lexer"""
    def __init__(self, tokens):
        self._tokens = list(tokens)
        for (idx, v) in enumerate(self._tokens):
            setattr(self, v.upper(), idx)

    def to_string(self, name):
        return self._tokens[name]

    def of_string(self, name):
        return getattr(self, name.upper())


Token = _TokHelper(_tokens)


def ident(scanner, token):
    """Takes the input a string that is either a scanner or a """
    if token in _keywords:
        return Token.of_string(token), None
    return Token.IDENTIFIER, token

scanner = re.Scanner([
    (r"\s+", None),
    (r"/\*(\n|.)*?\*/", None),  # comments
    (r"%.*\n", None),  # c ignore files
    (r"//.*\n", None),  # comments
    (r"[0-9]+", lambda scanner, token: (Token.INT, int(token))),
    (r"[A-Za-z_][A-Za-z_0-9]*", ident),
    (r"\[", lambda scanner, token: (Token.OBRACK, None)),
    (r"\]", lambda scanner, token: (Token.CBRACK, None)),
    (r";", lambda scanner, token: (Token.SEMI, None)),
    (r"{", lambda scanner, token: (Token.OCURL, None)),
    (r"}", lambda scanner, token: (Token.CCURL, None)),
    (r"=", lambda scanner, token: (Token.EQ, None)),
    (r",", lambda scanner, token: (Token.COMMA, None)),
    (r"<>", lambda scanner, token: (Token.CARRETS, None)),
    (r"\*", lambda scanner, token: (Token.STAR, None)),
    (r"\(", lambda scanner, token: (Token.OPAR, None)),
    (r"\)", lambda scanner, token: (Token.CPAR, None)),
    (r":", lambda scanner, token: (Token.COLON, None))
])


class PeekableIterator:
    """ An adaptor around an iterator that adds a lookahead of one element"""

    class Done(Exception):
        """Tried to peek when all the values had already been read"""
        pass

    def __init__(self, it):
        self._done = False
        self._it = it
        self._reload()

    def _reload(self):
        if self._done:
            return
        try:
            self._next = self._it.next()
        except StopIteration:
            self._next = None
            self._done = True

    def next(self):
        if self._done:
            raise StopIteration
        res = self._next
        self._reload()
        return res

    def peek(self):
        if self._done:
            raise Done
        return self._next

    def done(self):
        return self._done

    def __iter__(self):
        return self


def scan(file):
    with open(file, "r") as fd:
        data = fd.read()
    results, remainder = scanner.scan(data)
    assert remainder == ""
    results.append((Token.EOF, None))
    return PeekableIterator(iter(results))

###############################################################################


class LeafType:
    def iter_children(self):
        return
        yield None


class TypeStatement(LeafType):
    def __init__(self):
        self.parents = []


class TypeModifier:
    def get_base(self):
        if isinstance(self.elt, TypeModifier):
            return self.elt.get_base()
        return self.elt


class Tyref:
    def __init__(self, name):
        self.name = name

    def __str__(self):
        return self.name

    def iter_children(self):
        assert False


class NativeType(LeafType):
    def __init__(self, name):
        self.name = name

    def __str__(self):
        return self.name


class List(TypeModifier):
    def __init__(self, elt):
        self.elt = elt

    def __str__(self):
        return "[%s]" % (self.elt)


class Option(TypeModifier):
    def __init__(self, elt):
        self.elt = elt

    def __str__(self):
        return "*%s" % (self.elt)


class TypeAlias(TypeStatement):
    def __init__(self, name, target):
        TypeStatement.__init__(self)
        self.name = name
        self.target = target

    def __str__(self):
        return "%s : %s" % (self.name, self.target)

    def iter_children(self):
        assert False


class Void(LeafType):
    def __str__(self):
        return "void"

Void = Void()


class Enum(TypeStatement):
    def __init__(self, name, cases):
        TypeStatement.__init__(self)
        self.name = name
        self.cases = []
        self._lut = {}
        curr_val = 0
        for case_name, val in cases:
            if val is not None:
                curr_val = val
            self.cases.append((case_name, curr_val))
            self._lut[case_name] = val
            curr_val += 1

    def __str__(self):
        res = "enum %s {\n" % self.name
        for name, val in self.cases:
            res += "  %s = %i\n" % (name, val)
        res += "}"
        return res

    def __getitem__(self, v):
        return self._lut[case_name]


class Struct(TypeStatement):
    def __init__(self, name, fields):
        TypeStatement.__init__(self)
        self.name = name
        self.fields = fields

    def __str__(self):
        res = "struct %s {\n" % self.name
        for f in self.fields:
            res += "  " + str(f) + "\n"
        res += "}"
        return res

    def iter_children(self):
        for bdg in self.fields:
            yield bdg.target


class Union(TypeStatement):
    # reversed(range(10))

    def __init__(self, name, bdg, fields):
        TypeStatement.__init__(self)
        self.binding = bdg
        self.name = name
        self.default = None
        self.fields = []
        acc = []
        for k, v in fields:
            if k is None:
                assert v is not None
                self.default = v
            else:
                acc.append(k)
                if v is not None:
                    self.fields.append((acc, v))
                    acc = []
        assert acc == []

    def __str__(self):
        res = "union %s {\n" % self.name
        for k, v in self.fields:
            res += "case " + ", ".join(k) + ":\n  " + str(v) + "\n"
        if self.default is not None:
            res += "default:\n  " + str(self.default) + "\n"
        res += "}"
        return res

    def iter_children(self):
        yield self.binding.target
        for k, bdg in self.fields:
            if bdg is not Void:
                yield bdg.target
        if self.default:
            if self.default is not Void:
                yield bdg.default.target

###############################################################################

def consume(scanner, *args):
    for expected in args:
        tok, _ = scanner.next()
        assert tok in expected, "parse error got: %s" % Token.to_string(tok)


def ident(scanner):
    tok, val = scanner.next()
    assert tok == Token.IDENTIFIER, \
        "Got %s, %s" % (Token.to_string(tok), str(val))
    return val


def number(scanner):
    tok, val = scanner.next()
    assert tok == Token.INT
    return val


def get_list(scanner, f, end, sep=None):
    while True:
        yield f(scanner)
        token, _ = scanner.peek()
        if token == end:
            scanner.next()
            return
        if sep is not None:
            assert token == sep
            scanner.next()


class Dispatcher:
    """
    A helper class that takes a list of functions (one per token type) and
    dispatches to the call to the right class. It also offers a singleton
    pattern to call the dispatch method implicitly.
    """

    def dispatch(self, scanner, **kwargs):
        """Visit a node."""
        token, val = scanner.next()
        name = Token.to_string(token)
        f = getattr(self, name, None)
        assert f is not None, \
            "Missing method: %s.%s" % (self.__class__.__name__, name)
        if val is None:
            return f(scanner, **kwargs)
        else:
            return f(scanner, val, **kwargs)

    @classmethod
    def f(cls, scanner, **kwargs):
        return cls().dispatch(scanner, **kwargs)


# typedef, field in a struct or case in a union. Consumes the scanner until
# the trailing semi-colon.
class TypeBinding(Dispatcher):

    def unsigned(self, scanner, end=Token.SEMI):
        next_tok, next_val = scanner.peek()
        assert next_tok == Token.IDENTIFIER
        if next_val in _int_types:
            scanner.next()
        return self.identifier(scanner, "hyper", end)

    def identifier(self, scanner, tgt, end=Token.SEMI):
        if tgt in _int_types:
            tgt = "int"
        star = False
        if scanner.peek()[0] == Token.STAR:
            star = True
            scanner.next()
        name = ident(scanner)
        if tgt in _native_types:
            ty = NativeType(tgt)
        else:
            ty = Tyref(tgt)
        tok, val = scanner.next()
        if star:
            ty = Option(ty)
            assert tok == end
        elif tok == end:
            pass
        elif tok == Token.CARRETS:
            if tgt != "string":
                ty = List(ty)
            consume(scanner, [end])
        elif tok == Token.OBRACK:
            if tgt != "opaque":
                ty = List(ty)
            consume(scanner,
                    [Token.INT, Token.IDENTIFIER],
                    [Token.CBRACK],
                    [end])
        else:
            assert False
        return TypeAlias(name, ty)


class VoidableTypeBinding(TypeBinding):
    def void(sef, scanner, end=Token.SEMI):
        consume(scanner, [end])
        return Void


def enum_case(scanner):
    name = ident(scanner)
    num = None
    if scanner.peek()[0] == Token.EQ:
        scanner.next()
        num = number(scanner)
    return name, num


class UnionCase(Dispatcher):

    def case(self, scanner):
        name = ident(scanner)
        consume(scanner, [Token.COLON])
        if scanner.peek()[0] in [Token.CASE, Token.DEFAULT]:
            return name, None
        # TODO: peek to make sure there's a binding
        binding = VoidableTypeBinding.f(scanner)
        return name, binding

    def default(self, scanner):
        consume(scanner, [Token.COLON])
        return None, VoidableTypeBinding.f(scanner)


class TopLevel(Dispatcher):
    def typedef(self, scanner):
        return TypeBinding.f(scanner, end=Token.SEMI)

    def enum(self, scanner):
        name = ident(scanner)
        consume(scanner, [Token.OCURL])
        cases = list(get_list(scanner, enum_case, Token.CCURL, Token.COMMA))
        consume(scanner, [Token.SEMI])
        return Enum(name, cases)

    def struct(self, scanner):
        name = ident(scanner)
        consume(scanner, [Token.OCURL])
        fields = list(get_list(scanner, TypeBinding.f, Token.CCURL))
        consume(scanner, [Token.SEMI])
        return Struct(name, fields)

    def union(self, scanner):
        name = ident(scanner)
        consume(scanner, [Token.SWITCH], [Token.OPAR])
        bdg = TypeBinding.f(scanner, end=Token.CPAR)
        consume(scanner, [Token.OCURL])
        cases = list(get_list(scanner, UnionCase.f, Token.CCURL))
        consume(scanner, [Token.SEMI])
        return Union(name, bdg, cases)

    def namespace(self, scanner):
        consume(scanner, [Token.IDENTIFIER], [Token.OCURL])
        level = 1
        while level > 0:
            tok, _ = scanner.next()
            if tok == Token.OCURL:
                level += 1
            elif tok == Token.CCURL:
                level -= 1
        consume(scanner, [Token.SEMI])
        # We discard namespaces here...
        return Void

    def eof(self, scanner):
        return None


def parse_prot_file(filename):
    """Parses a prot file, the result is a iterator over the definitions in
    that file."""
    scanner = scan(filename)
    while True:
        v = TopLevel.f(scanner)
        if v is None:
            return
        elif v is not Void:
            yield v

###############################################################################


class StubEmitter:
    """ Please document me... """

    def get_base(self, ty):
        while isinstance(ty, TypeModifier):
            ty = ty.elt
        return ty

    def unwind(self, ty, parent):
        if isinstance(ty, TypeAlias):
            ty = self.unwind(ty.target, parent)
        elif isinstance(ty, TypeModifier):
            ty.elt = self.unwind(ty.elt, parent)
        elif isinstance(ty, Tyref):
            ty = self.unwind(self._defs[ty.name], parent)
        elif isinstance(ty, TypeStatement):
            ty.parents.append(parent)
        return ty

    def _simplify_field(self, parent, fld):
        fld.target = self.unwind(fld.target, parent)

    def __init__(self, prot_file):
        self._defs = {}  # the definitions in the prot file that we parsed
        self._filename = prot_file

        for x in parse_prot_file(prot_file):
            self._defs[x.name] = x

        # We flatten all the typeref and complete all the unions (that replace
        # the default values with cases)...
        for k in self._defs:
            v = self._defs[k]
            if isinstance(v, Struct):
                for b in v.fields:
                    self._simplify_field(v, b)
            elif isinstance(v, Union):
                if v.default is not None:
                    tags_ty = self._defs[v.binding.target.name]
                    remainding_tags = set((k for k, _ in tags_ty.cases))
                    for keys, _ in v.fields:
                        for k in keys:
                            remainding_tags.remove(k)
                    v.fields.append((list(remainding_tags), v.default))
                    v.default = None
                for _, b in v.fields:
                    if b is not Void:
                        self._simplify_field(v, b)
                self._simplify_field(v, v.binding)

            elif isinstance(v, TypeAlias):
                pass
            else:
                assert isinstance(v, Enum)
        self._emited = set()

    def mk_fld_name(self, v):
        if v == "lambda":
            return "body"
        return v

    def mk_enum_name(self, v, prefix):
        if prefix is not None:
            v = v[len(prefix):]
        return v.upper()

    def find_enum_prefix(self, v):
        """ Takes all the cases of an enum and finds the prefix that is common
        to all of them and should be stripped out."""
        if len(v.cases) < 2:
            return None
        elts = [x.split("_") for x, _ in v.cases]
        minlen = min(len(e) for e in elts)
        res = []
        for i in range(0, minlen - 1):
            common = elts[0][i]
            all_same = all(x[i] == common for x in elts[1:])
            if all_same:
                res.append(common)
            else:
                break
        return None if res == [] else "_".join(res)+"_"

    def cleanup_ty_name(self, v):
        v = v.lower()
        elts = v.split("_")
        if elts[-1] == "t":
            elts = elts[:-1]
        if elts[0] == "xpub3":
            elts = elts[1:]
        return elts

    def mk_class_name(self, v):
        return "".join([s.capitalize() for s in self.cleanup_ty_name(v)])

    def mk_fn_name(self, v):
        return "mk_"\
            + "_".join([s for s in self.cleanup_ty_name(v)])

    def class_name_to_fnname(self, v):
        # camel case to underscore separated
        underscore = re.sub('(?!^)([A-Z]+)', r'_\1', v).lower()
        return "mk_" + underscore

    def build_val(self, ty, src):
        base = self.get_base(ty)
        if isinstance(base, NativeType):
            return src
        if isinstance(ty, List):
            return "[%s for e in %s]" % (self.build_val(ty.elt, "e"), src)
        elif isinstance(ty, Option):
            return "%s if %s != [] else None" %\
                (self.build_val(ty.elt, "%s[0]" % src), src)
        elif isinstance(ty, Struct) or isinstance(base, Enum):
            return "%s(%s)" % (self.mk_class_name(ty.name), src)
        elif isinstance(ty, Union):
            return "%s(%s)" % (self.mk_fn_name(ty.name), src)
        else:
            assert False

    def get_accessor(self, bdg, name="json_val"):
        src = "%s[\"%s\"]" % (name, bdg.name)
        return self.build_val(bdg.target, src)

    def emit(self, name):
        tgt = self._defs[name]
        assert name in self._defs, "Missing definition for %s" % name
        tgt = self._defs[name]
        print '''
"""
This file was automatically generated by ast_gen from %s. It contains all the
class required to represent the type `%s` in python and helper functions to
create them from a `json` representation.
"""
from enum import Enum  #enum34

class Node:
    _fields = []
    pass
''' % (os.path.basename(self._filename), name)
        self.emit_ty(tgt)

    def emit_deps(self, ty):
        for c in ty.iter_children():
            ty = self.get_base(c)
            if isinstance(ty, NativeType) or ty is Void:
                pass
            else:
                self.emit_ty(ty)

    def emit_ty(self, ty, classname=None, parentname=None):
        if classname is None:
            classname = self.mk_class_name(ty.name)
            if ty.name in self._emited:
                return
            self._emited.add(ty.name)
        if parentname is None:
            parentname = "Node"
        method = 'emit_' + ty.__class__.__name__.lower()
        f = getattr(self, method, None)
        assert f is not None, "Missing emitter %s" % method
        f(ty, classname, parentname)

    def emit_struct(self, ty, classname, parentname):
        print "class %s(%s):" % (classname, parentname)
        fields = ['"%s"' % self.mk_fld_name(fld.name) for fld in ty.fields]
        print "    _fields = [%s]" % ", ".join(fields)
        print "    __slots__ = [%s]" % ", ".join(fields)
        print
        print "    def __init__(self, json_val):"
        for fld in ty.fields:
            fld_name = self.mk_fld_name(fld.name)
            print "        self.%s = %s" % (fld_name, self.get_accessor(fld))
        print
        print "def %s(json_val):" % self.class_name_to_fnname(classname)
        print "    return %s(json_val)" % classname
        print
        self.emit_deps(ty)

    def emit_void(self, tgt, classname, parentname):
        print "class %s(%s):" % (classname, parentname)
        print "    __slots__ = []"
        print
        print "def mk_%s(json_val):" % classname.lower()
        print "    return %s()" % classname
        print

    def emit_union(self, ty, classname, parentname):
        print "class %s(%s):" % (classname, parentname)
        # Assert that this Enum is only referenced in this union
        assert ty.binding.target.parents == [ty]
        assert isinstance(ty.binding.target, Enum)
        assert ty.default is None
        print "    __slots__ = []"
        enum_prefix = self.find_enum_prefix(ty.binding.target)
        for k, v in ty.binding.target.cases:
            print "    %s = %i" % (self.mk_enum_name(k, enum_prefix), v)
        print

        print "def %s(json_val):" % self.class_name_to_fnname(classname)
        print "    tag = json_val['%s']" % ty.binding.name

        first = True

        for k, bdg in ty.fields:
            for v in k:
                cond = "if" if first else "elif"
                first = False
                enum_name = self.mk_enum_name(v, enum_prefix)
                print "    %s tag == %s.%s:" % (cond, classname, enum_name)
                subclass_name = self.mk_class_name(v)
                if bdg is Void:
                    print "        return %s()" % subclass_name
                else:
                    print "        return %s(json_val['%s'])" %\
                        (subclass_name, bdg.name)
        assert not first
        print "    else:"
        print "        assert False, 'Unknown tag %i' % tag"
        print

        for k, bdg in ty.fields:
            subtype = Void if bdg is Void else bdg.target
            for v in k:
                subclass = self.mk_class_name(v)
                self.emit_ty(subtype, subclass, classname)

    def emit_enum(self, tgt, classname, parentname):
        print "class %s(%s, Enum):" % (classname, parentname)
        prefix = self.find_enum_prefix(tgt)
        for k, v in tgt.cases:
            print "    %s = %i" % (self.mk_enum_name(k, prefix), v)
        print
        print "def %s(json_val):" % self.class_name_to_fnname(classname)
        print "    return %s(json_val)" % classname

        #print tgt

    def emit_typealias(self, tgt, classname, parentname):
        assert False


def main():
    v = StubEmitter(sys.argv[1])
    v.emit(sys.argv[2])

main()
