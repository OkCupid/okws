
#include "zstr.h"
#include "pub3.h"
#include "pub3file.h"

// XDR functions for pub3 objects

//-----------------------------------------------------------------------

void
zstr_to_xdr (const zstr &z, xpub3_zstr_t *x, int l)
{
  x->s = z.to_str ();
  if (l != Z_DISABLE) x->zs = z.compress (l);
  x->clev = l;
}

//-----------------------------------------------------------------------

zstr
xdr_to_zstr (const xpub3_zstr_t &x)
{
  str zs (x.zs.base (), x.zs.size ()); 
  return zstr (x.s, zs, x.clev);
}

//-----------------------------------------------------------------------

static void 
expr_to_rpc_ptr (const pub3::expr_t *e, rpc_ptr<xpub3_expr_t> *x)
{
  if (e) {
    x->alloc ();
    e->to_xdr (*x);
  }
}

//-----------------------------------------------------------------------

static void
expr_to_xdr (ptr<const pub3::expr_t> e, xpub3_expr_t *x)
{
  if (e) { e->to_xdr (x); }
  else   { x->set_typ (XPUB3_EXPR_NULL); }
}

//-----------------------------------------------------------------------

static ptr<pub3::identifier_list_t>
xdr_to_idlist (const xpub3_identifier_list_t &x)
{
  ptr<pub3::identifier_list_t> ret = New refcounted<pub3::identifier_list_t> ();
  for (size_t i = 0; i < x.size (); i++) {
    ret->push_back (x[i]);
  }
  return ret;
}

//-----------------------------------------------------------------------

static void
idlist_to_xdr (const pub3::identifier_list_t &in, xpub3_identifier_list_t *out)
{
  for (size_t i = 0; i < in.size (); i++){
    out->push_back (in[i]);
  }
}

//-----------------------------------------------------------------------

bool
pub3::for_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_FOR);
  x->for_statement->lineno = lineno ();
  x->for_statement->iter = _iter;
  expr_to_xdr (_arr, &x->for_statement->arr);
  if (_body) { _body->to_xdr (&x->for_statement->body); }
  if (_empty) {
    x->for_statement->empty.alloc ();
    _empty->to_xdr (x->for_statement->empty);
  }
  return true;
}

//-----------------------------------------------------------------------

pub3::for_t::for_t (const xpub3_for_t &x)
  : statement_t (x.lineno),
    _iter (x.iter),
    _arr (expr_t::alloc (x.arr)),
    _body (zone_t::alloc (x.body)),
    _empty (zone_t::alloc (x.empty)) {}

//-----------------------------------------------------------------------

pub3::if_clause_t::if_clause_t (const xpub3_if_clause_t &x)
  : _lineno (x.lineno),
    _expr (expr_t::alloc (x.expr)),
    _body (zone_t::alloc (x.body)) {}

//-----------------------------------------------------------------------

pub3::if_t::if_t (const xpub3_if_t &x) : statement_t (x.lineno)
{
  if (x.clauses.size ()) {
    _clauses = New refcounted<if_clause_list_t> ();
    for (size_t i = 0; i < x.clauses.size (); i++) {
      _clauses->push_back (New refcounted<if_clause_t> (x.clauses[i]));
    }
  }
}

//-----------------------------------------------------------------------

pub3::include_t::include_t (const xpub3_include_t &x)
  : statement_t (x.lineno),
    _file (expr_t::alloc (x.file)),
    _dict (expr_t::alloc (x.dict)) {}

//-----------------------------------------------------------------------

pub3::load_t::load_t (const xpub3_include_t &x)
  : include_t (x) {}

//-----------------------------------------------------------------------

bool
pub3::include_t::to_xdr_base (xpub3_statement_t *x, 
			      xpub3_statement_typ_t typ) const
{
  x->set_typ (typ);
  x->include->lineno = lineno ();
  expr_to_xdr (_file, &x->include->file);
  expr_to_xdr (_dict, &x->include->dict);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::include_t::to_xdr (xpub3_statement_t *x) const
{
  return to_xdr_base (x, XPUB3_STATEMENT_INCLUDE);
}

//-----------------------------------------------------------------------

bool
pub3::load_t::to_xdr (xpub3_statement_t *x) const
{
  return to_xdr_base (x, XPUB3_STATEMENT_LOAD);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_t::alloc (const xpub3_expr_t *x)
{
  ptr<pub3::expr_t> ret;
  if (x) ret = expr_t::alloc (*x);
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::statement_t>
pub3::statement_t::alloc (const xpub3_statement_t &x)
{
  ptr<pub3::statement_t> r;
  switch (x.typ) {
  case XPUB3_STATEMENT_NONE: 
    break;
  case XPUB3_STATEMENT_INCLUDE: 
    r = New refcounted<include_t> (*x.include); 
    break;
  case XPUB3_STATEMENT_LOAD:
    r = New refcounted<load_t> (*x.include); 
    break;
  case XPUB3_STATEMENT_ZONE:
    r = New refcounted<statement_zone_t> (*x.zone);
    break;
  case XPUB3_STATEMENT_FOR:
    r = New refcounted<for_t> (*x.for_statement);
    break;
  case XPUB3_STATEMENT_LOCALS:
    r = New refcounted<locals_t> (*x.decls);
    break;
  case XPUB3_STATEMENT_GLOBALS:
    r = New refcounted<globals_t> (*x.decls);
    break;
  case XPUB3_STATEMENT_UNIVERSALS:
    r = New refcounted<universals_t> (*x.decls);
    break;
  case XPUB3_STATEMENT_IF:
    r = New refcounted<if_t> (*x.if_statement);
    break;
  case XPUB3_STATEMENT_PRINT:
    r = New refcounted<print_t> (*x.print);
    break;
  case XPUB3_EXPR_STATEMENT:
    r = New refcounted<expr_statement_t> (*x.expr_statement);
    break;
  case XPUB3_STATEMENT_FNDEF:
    r = New refcounted<fndef_t> (*x.fndef);
    break;
  case XPUB3_STATEMENT_SWITCH:
    r = New refcounted<switch_t> (*x.switch_statement);
    break;
  case XPUB3_STATEMENT_BREAK:
    r = New refcounted<break_t> (*x.break_statement);
    break;
  case XPUB3_STATEMENT_CONTINUE:
    r = New refcounted<continue_t> (*x.continue_statement);
    break;
  case XPUB3_STATEMENT_RETURN:
    r = New refcounted<return_t> (*x.return_statement);
    break;
  default: 
    break;
  }
  return r;
}

//-----------------------------------------------------------------------

ptr<pub3::zone_t>
pub3::zone_t::alloc (const xpub3_zone_t &z)
{
  ptr<pub3::zone_t> r;
  switch (z.typ) {
  case XPUB3_ZONE_HTML:
    r = zone_html_t::alloc (*z.html);
    break;
  case XPUB3_ZONE_TEXT:
    r = zone_text_t::alloc (*z.text);
    break;
  case XPUB3_ZONE_INLINE_EXPR:
    r = zone_inline_expr_t::alloc (*z.zone_inline);
    break;
  case XPUB3_ZONE_PUB:
    r = zone_pub_t::alloc (*z.zone_pub);
    break;
  default:
    break;
  }
  return r;
}

//-----------------------------------------------------------------------

ptr<pub3::zone_t>
pub3::zone_t::alloc (const xpub3_zone_t *z)
{
  ptr<zone_t> r;
  if (z) { r = alloc (*z); }
  return r;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_t::alloc (const xpub3_expr_t &x)
{
  ptr<pub3::expr_t> r;
  switch (x.typ) {
  case XPUB3_EXPR_PUBNULL:
    r = expr_null_t::alloc () ;
    break;
  case XPUB3_EXPR_MATHOP:
    r = expr_mathop_t::alloc (*x.mathop);
    break;
  case XPUB3_EXPR_NOT:
    r = New refcounted<pub3::expr_NOT_t> (*x.xnot);
    break;
  case XPUB3_EXPR_CALL:
    r = call_t::alloc (*x.call);
    break;
  case XPUB3_EXPR_RELATION:
    r = New refcounted<pub3::expr_relation_t> (*x.relation);
    break;
  case XPUB3_EXPR_DICT:
    r = New refcounted<pub3::expr_dict_t> (*x.dict);
    break;
  case XPUB3_EXPR_LIST:
    r = New refcounted<pub3::expr_list_t> (*x.list);
    break;
  case XPUB3_EXPR_EQ:
    r = New refcounted<pub3::expr_EQ_t> (*x.eq);
    break;
  case XPUB3_EXPR_DICTREF:
    r = New refcounted<pub3::expr_dictref_t> (*x.dictref);
    break;
  case XPUB3_EXPR_VECREF:
    r = New refcounted<pub3::expr_vecref_t> (*x.vecref);
    break;
  case XPUB3_EXPR_REF:
    r = New refcounted<pub3::expr_varref_t> (*x.xref);
    break;
  case XPUB3_EXPR_STR:
    r = New refcounted<pub3::expr_str_t> (*x.xstr);
    break;
  case XPUB3_EXPR_SHELL_STR:
    r = New refcounted<pub3::expr_shell_str_t> (*x.shell_str);
    break;
  case XPUB3_EXPR_INT:
    r = New refcounted<pub3::expr_int_t> (*x.xint);
    break;
  case XPUB3_EXPR_UINT:
    r = New refcounted<pub3::expr_uint_t> (*x.xuint);
    break;
  case XPUB3_EXPR_DOUBLE:
    r = New refcounted<pub3::expr_double_t> (*x.xdouble);
    break;
  case XPUB3_EXPR_REGEX:
    r = New refcounted<pub3::expr_regex_t> (*x.regex);
    break;
  case XPUB3_EXPR_ASSIGNMENT:
    r = New refcounted<pub3::expr_assignment_t> (*x.assignment);
    break;
  case XPUB3_EXPR_BOOL:
    r = pub3::expr_bool_t::alloc (x.xbool->val);
    break;
  case XPUB3_EXPR_LAMBDA:
    r = pub3::lambda_t::alloc (*x.lambda);
    break;
  default:
    break;
  }
  return r;
}

//-----------------------------------------------------------------------

pub3::expr_OR_t::expr_OR_t (const xpub3_mathop_t &x)
  : expr_logical_t (x.lineno),
    _t1 (expr_t::alloc (x.o1)),
    _t2 (expr_t::alloc (x.o2)) {}

//-----------------------------------------------------------------------

pub3::expr_AND_t::expr_AND_t (const xpub3_mathop_t &x)
  : expr_logical_t (x.lineno),
    _f1 (expr_t::alloc (x.o1)),
    _f2 (expr_t::alloc (x.o2)) {}

//-----------------------------------------------------------------------

pub3::expr_add_t::expr_add_t (const xpub3_mathop_t &x)
  : expr_binaryop_t (expr_t::alloc (x.o1), expr_t::alloc (x.o2), x.lineno),
    _pos (x.opcode == XPUB3_MATHOP_ADD) {}

//-----------------------------------------------------------------------

pub3::expr_mult_t::expr_mult_t (const xpub3_mathop_t &x)
  : expr_binaryop_t (expr_t::alloc (x.o1), expr_t::alloc (x.o2), x.lineno) {}

//-----------------------------------------------------------------------

pub3::expr_div_t::expr_div_t (const xpub3_mathop_t &x)
  : expr_div_or_mod_t (expr_t::alloc (x.o1), expr_t::alloc (x.o2), x.lineno) {}

//-----------------------------------------------------------------------

pub3::expr_mod_t::expr_mod_t (const xpub3_mathop_t &x)
  : expr_div_or_mod_t (expr_t::alloc (x.o1), expr_t::alloc (x.o2), x.lineno) {}

//-----------------------------------------------------------------------

pub3::expr_NOT_t::expr_NOT_t (const xpub3_not_t &x)
  : expr_logical_t (x.lineno),
    _e (expr_t::alloc (x.e)) {}

//-----------------------------------------------------------------------

pub3::expr_list_t::expr_list_t (const xpub3_expr_list_t &x)
  : expr_t (x.lineno)
{
  setsize (x.list.size ());
  for (size_t i = 0; i < x.list.size (); i++) {
    (*this)[i] = pub3::expr_t::alloc (x.list[i]);
  }
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_LIST);
  return to_xdr (x->list);
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::to_xdr (xpub3_expr_list_t *l) const
{
  l->lineno = _lineno;
  l->list.setsize (size ());
  for (size_t i = 0; i < size (); i++) {
    ptr<const pub3::expr_t> x = (*this)[i];
    if (x) {
      x->to_xdr (&l->list[i]);
    } else {
      l->list[i].set_typ (XPUB3_EXPR_PUBNULL);
    }
  }
  return true;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
pub3::expr_list_t::alloc (const xpub3_expr_list_t *x)
{
  ptr<expr_list_t> ret;
  if (x) ret = alloc (*x);
  return ret;
}

//-----------------------------------------------------------------------

pub3::expr_dictref_t::expr_dictref_t (const xpub3_dictref_t &x)
  : expr_t (x.lineno),
    _dict (expr_t::alloc (x.dict)),
    _key (x.key) {}

//-----------------------------------------------------------------------

pub3::expr_vecref_t::expr_vecref_t (const xpub3_vecref_t &x)
  : expr_t (x.lineno),
    _vec (expr_t::alloc (x.vec)),
    _index (expr_t::alloc (x.index)) {}

//-----------------------------------------------------------------------

pub3::expr_varref_t::expr_varref_t (const xpub3_ref_t &x)
  : expr_t (x.lineno), _name (x.key) {}

//-----------------------------------------------------------------------

pub3::expr_relation_t::expr_relation_t (const xpub3_relation_t &x)
  : expr_logical_t (x.lineno),
    _l  (expr_t::alloc (x.left)),
    _r  (expr_t::alloc (x.right)),
    _op (x.relop) {}

//-----------------------------------------------------------------------

pub3::expr_EQ_t::expr_EQ_t (const xpub3_eq_t &x)
  : expr_logical_t (x.lineno),
    _o1  (expr_t::alloc (x.o1)),
    _o2  (expr_t::alloc (x.o2)),
    _pos (x.pos) {}

//-----------------------------------------------------------------------

pub3::expr_str_t::expr_str_t (const xpub3_str_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_int_t::expr_int_t (const xpub3_int_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_uint_t::expr_uint_t (const xpub3_uint_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_double_t::expr_double_t (const xpub3_double_t &x)
  : _val (convertdouble (x.val)) {}

//-----------------------------------------------------------------------

bool
pub3::expr_double_t::to_xdr (xpub3_double_t *x) const
{
#define BUFSZ 128
  char buf[BUFSZ];
  snprintf (buf, BUFSZ, "%g", _val);
  x->val = buf;
#undef BUFSZ
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_AND_t::to_xdr (xpub3_expr_t *x) const
{
  return expr_mathop_t::to_xdr (x, XPUB3_MATHOP_AND, _f1, _f2, _lineno);
}

//-----------------------------------------------------------------------

bool
pub3::expr_OR_t::to_xdr (xpub3_expr_t *x) const
{
  return expr_mathop_t::to_xdr (x, XPUB3_MATHOP_OR, _t1, _t2, _lineno);
}

//-----------------------------------------------------------------------

bool
pub3::expr_binaryop_t::to_xdr (xpub3_expr_t *x) const
{
  return expr_mathop_t::to_xdr (x, opcode (), _o1, _o2, _lineno);
}

//-----------------------------------------------------------------------

xpub3_mathop_opcode_t pub3::expr_add_t::opcode () const
{ return _pos ? XPUB3_MATHOP_ADD : XPUB3_MATHOP_SUBTRACT; }
xpub3_mathop_opcode_t pub3::expr_mult_t::opcode () const
{ return XPUB3_MATHOP_MULT; }
xpub3_mathop_opcode_t pub3::expr_div_t::opcode () const
{ return XPUB3_MATHOP_DIV; }
xpub3_mathop_opcode_t pub3::expr_mod_t::opcode () const
{ return XPUB3_MATHOP_MOD; }

//-----------------------------------------------------------------------

bool
pub3::expr_NOT_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_NOT);
  x->xnot->lineno = _lineno;
  expr_to_rpc_ptr (_e, &x->xnot->e);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::call_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_CALL);
  x->call->lineno = _lineno;
  expr_to_rpc_ptr (_fn, &x->call->fn);
  if (args ()) {
    args ()->to_xdr (&x->call->args);
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_relation_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_RELATION);
  x->relation->lineno = _lineno;
  x->relation->relop = _op;
  expr_to_rpc_ptr (_l, &x->relation->left);
  expr_to_rpc_ptr (_r, &x->relation->right);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_EQ_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_EQ);
  x->eq->lineno = _lineno;
  x->eq->pos = _pos;
  expr_to_rpc_ptr (_o1, &x->eq->o1);
  expr_to_rpc_ptr (_o2, &x->eq->o2);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_dictref_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_DICTREF);
  x->dictref->lineno = _lineno;
  x->dictref->key = _key;
  expr_to_rpc_ptr (_dict, &x->dictref->dict);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_vecref_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_VECREF);
  x->vecref->lineno = _lineno;
  expr_to_rpc_ptr (_index, &x->vecref->index);
  expr_to_rpc_ptr (_vec, &x->vecref->vec);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_varref_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_REF);
  x->xref->lineno = _lineno;
  x->xref->key = _name;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_STR);
  x->xstr->val = _val;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_int_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_INT);
  x->xint->val = _val;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_uint_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_UINT);
  x->xuint->val = _val;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_double_t::to_xdr (xpub3_expr_t *x) const
{
#define BUFSZ 64
  x->set_typ (XPUB3_EXPR_DOUBLE);
  char buf[BUFSZ];
  snprintf (buf, BUFSZ, "%g", _val);
  x->xdouble->val = buf;
  return true;
#undef BUFSZ
}

//-----------------------------------------------------------------------

bool
pub3::if_clause_t::to_xdr (xpub3_if_clause_t *x) const
{
  x->lineno = _lineno;
  expr_to_xdr (_expr, &x->expr);
  if (_body) { _body->to_xdr (&x->body); }

  return true;
}

//-----------------------------------------------------------------------

bool
pub3::if_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_IF);
  x->if_statement->lineno = lineno ();

  size_t s = _clauses ? _clauses->size () : size_t (0);
  x->if_statement->clauses.setsize (s);
  for (size_t i = 0; i < s; i++) {
    (*_clauses)[i]->to_xdr (&x->if_statement->clauses[i]);
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_shell_str_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_SHELL_STR);
  x->shell_str->lineno = _lineno;
  if (_els) {
    _els->to_xdr (&x->shell_str->elements);
  }

  return true;
}

//-----------------------------------------------------------------------

pub3::expr_shell_str_t::expr_shell_str_t (const xpub3_shell_str_t &x)
  : expr_t (x.lineno),
    _els (expr_list_t::alloc (x.elements)) {}

//-----------------------------------------------------------------------

pub3::expr_dict_t::expr_dict_t (const xpub3_dict_t &x)
  : expr_t (x.lineno)
{
  size_t lim = x.entries.size ();
  for (size_t i = 0; i < lim; i++) {
    add (binding_t (x.entries [i]));
  }
}

//-----------------------------------------------------------------------

ptr<pub3::expr_dict_t>
pub3::expr_dict_t::alloc (const xpub3_dict_t &d)
{ return New refcounted<expr_dict_t> (d); }

//-----------------------------------------------------------------------

pub3::binding_t::binding_t (const xpub3_binding_t &x)
  : _name (x.key),
    _expr (expr_t::alloc(x.val)) {}

//-----------------------------------------------------------------------

bool
pub3::expr_dict_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_DICT);
  x->dict->lineno = _lineno;
  bindtab_t::const_iterator_t it (*this);
  const str *key;
  ptr<expr_t> val;

  while ((key = it.next (&val))) {
    xpub3_binding_t &b3 = x->dict->entries.push_back ();
    b3.key = *key;
    expr_to_rpc_ptr (val, &b3.val);
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::zone_inline_expr_t::to_xdr (xpub3_zone_t *x) const
{
  x->set_typ (XPUB3_ZONE_INLINE_EXPR);
  x->zone_inline->lineno = lineno ();
  expr_to_xdr (_expr, &x->zone_inline->expr);
  return true;
}

//-----------------------------------------------------------------------

pub3::zone_inline_expr_t::zone_inline_expr_t (const xpub3_zone_inline_expr_t &x)
  : zone_t (x.lineno), _expr (expr_t::alloc (x.expr)) {}

//-----------------------------------------------------------------------

ptr<pub3::zone_inline_expr_t> 
pub3::zone_inline_expr_t::alloc (const xpub3_zone_inline_expr_t &x)
{ return New refcounted<zone_inline_expr_t> (x); }

//-----------------------------------------------------------------------

pub3::expr_regex_t::expr_regex_t (const xpub3_regex_t &x)
  : expr_t (x.lineno),
    _body (x.body),
    _opts (x.opts)
{
  str err;
  if (!(_rxx = rxx_factory_t::compile (_body, _opts, &err))) {
    warn << "Unexpected regex compile error: " << err << "\n";
  }
}

//-----------------------------------------------------------------------

bool
pub3::expr_regex_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_REGEX);
  x->regex->lineno = _lineno;
  x->regex->body = _body;
  if (x->regex->opts) {
    x->regex->opts = _opts;
  }
  return true;
}

//-----------------------------------------------------------------------

pub3::print_t::print_t (const xpub3_print_t &x)
  : statement_t (x.lineno),
    _args (expr_list_t::alloc (x.args)) {}

//-----------------------------------------------------------------------

bool
pub3::print_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_PRINT);
  x->print->lineno = lineno ();
  if (_args) {
    _args->to_xdr (&x->print->args);
  }
  return true;
}

//-----------------------------------------------------------------------

pub3::expr_assignment_t::expr_assignment_t (const xpub3_assignment_t &x)
  : _lhs (expr_t::alloc (x.lhs)),
    _rhs (expr_t::alloc (x.rhs)),
    _lineno (x.lineno) {}

//-----------------------------------------------------------------------

bool 
pub3::expr_assignment_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_ASSIGNMENT);
  x->assignment->lineno = _lineno;
  expr_to_rpc_ptr (_lhs, &x->assignment->lhs);
  expr_to_rpc_ptr (_rhs, &x->assignment->rhs);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_mathop_t::to_xdr (xpub3_expr_t *out, xpub3_mathop_opcode_t code,
			     const expr_t *o1, const expr_t *o2,
			     int lineno)
{
  out->set_typ (XPUB3_EXPR_MATHOP);
  out->mathop->opcode = code;
  out->mathop->lineno = lineno;
  expr_to_rpc_ptr (o1, &out->mathop->o1);
  expr_to_rpc_ptr (o2, &out->mathop->o2);
  return true;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_mathop_t::alloc (const xpub3_mathop_t &op)
{
  ptr<expr_t> ret;
  switch (op.opcode) {
  case XPUB3_MATHOP_ADD:
  case XPUB3_MATHOP_SUBTRACT:
    ret = New refcounted<pub3::expr_add_t> (op);
    break;
  case XPUB3_MATHOP_OR:
    ret = New refcounted<pub3::expr_OR_t> (op);
    break;
  case XPUB3_MATHOP_AND:
    ret = New refcounted<pub3::expr_AND_t> (op);
    break;
  case XPUB3_MATHOP_MOD:
    ret = New refcounted<pub3::expr_mod_t> (op);
    break;
  case XPUB3_MATHOP_MULT:
    ret = New refcounted<pub3::expr_mult_t> (op);
    break;
  case XPUB3_MATHOP_DIV:
    ret = New refcounted<pub3::expr_div_t> (op);
    break;
  default:
    break;
  }
  return ret;
}

//-----------------------------------------------------------------------

pub3::expr_statement_t::expr_statement_t (const xpub3_expr_statement_t &x)
  : statement_t (x.lineno), _expr (expr_t::alloc (x.expr)) {}

//-----------------------------------------------------------------------

bool
pub3::expr_statement_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_EXPR_STATEMENT);
  x->expr_statement->lineno = lineno ();
  if (_expr) {
    x->expr_statement->expr.alloc ();
    _expr->to_xdr (x->expr_statement->expr);
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_bool_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_BOOL);
  x->xbool->val = int (_b);
  return true;
}

//-----------------------------------------------------------------------
 
bool
pub3::expr_strbuf_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_STR);
  x->xstr->val = _b;
  return true;
}
 
//-----------------------------------------------------------------------
 
pub3::case_t::case_t (const xpub3_case_t &x)
  : _lineno (x.lineno),
    _key (expr_t::alloc (x.key)),
    _zone (zone_t::alloc (x.body)) {}

//-----------------------------------------------------------------------

ptr<pub3::case_t>
pub3::case_t::alloc (const xpub3_case_t *x)
{
  ptr<case_t> ret;
  if (x) { ret = New refcounted<pub3::case_t> (*x); }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::case_t::to_xdr (xpub3_case_t *x) const
{
  x->lineno = _lineno;
  expr_to_rpc_ptr (_key, &x->key);
  if (_zone) { _zone->to_xdr (&x->body); }
  return true;
}

//-----------------------------------------------------------------------

pub3::case_list_t::case_list_t (const xpub3_cases_t &xl)
{
  for (size_t i = 0; i < xl.size (); i++) {
    push_back (New refcounted<case_t> (xl[i]));
  }
}

//-----------------------------------------------------------------------

ptr<pub3::case_list_t> pub3::case_list_t::alloc (const xpub3_cases_t &x)
{ return New refcounted<case_list_t> (x); } 

//-----------------------------------------------------------------------

bool
pub3::case_list_t::to_xdr (xpub3_cases_t *x) const
{
  x->setsize (size ());
  for (size_t i = 0; i < size (); i++) {
    (*this)[i]->to_xdr (&(*x)[i]);
  }
  return true;
}

//-----------------------------------------------------------------------

pub3::switch_t::switch_t (const xpub3_switch_t &x)
  : statement_t (x.lineno),
    _key (expr_t::alloc (x.key)),
    _cases (case_list_t::alloc (x.cases))
{
  populate_cases ();
}

//-----------------------------------------------------------------------

bool
pub3::switch_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_SWITCH);
  x->switch_statement->lineno = lineno ();
  expr_to_xdr (_key, &x->switch_statement->key);

  if (_cases) {
    _cases->to_xdr (&x->switch_statement->cases);
  }

  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_null_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_PUBNULL);
  return true;
}

//-----------------------------------------------------------------------

pub3::metadata_t::metadata_t (const xpub3_metadata_t &x)
  : _jfn (x.jailed_filename),
    _rfn (x.real_filename),
    _hsh (pub3::fhash_t::alloc (x.hash)),
    _toplev (false),
    _ctime (x.ctime) {}

//-----------------------------------------------------------------------

ptr<pub3::metadata_t> pub3::metadata_t::alloc (const xpub3_metadata_t &x)
{ return New refcounted<pub3::metadata_t> (x); }

//-----------------------------------------------------------------------

pub3::file_t::file_t (const xpub3_file_t &file, opts_t o)
  : _metadata (metadata_t::alloc (file.metadata)),
    _data_root (zone_t::alloc (file.root)),
    _opts (o) {}

//-----------------------------------------------------------------------

ptr<pub3::file_t> pub3::file_t::alloc (const xpub3_file_t &f, opts_t o)
{ return New refcounted<pub3::file_t> (f, o); }

//-----------------------------------------------------------------------

pub3::return_t::return_t (const xpub3_return_t &x)
  : statement_t (x.lineno),
    _val (expr_t::alloc (x.retval)) {}

//-----------------------------------------------------------------------

pub3::break_t::break_t (const xpub3_break_t &x)
  : statement_t (x.lineno) {}

//-----------------------------------------------------------------------

bool
pub3::break_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_BREAK);
  x->break_statement->lineno = lineno ();
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::return_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_RETURN);
  x->return_statement->lineno = lineno ();
  expr_to_xdr (_val, &x->return_statement->retval);
  return true;
}

//-----------------------------------------------------------------------

pub3::continue_t::continue_t (const xpub3_continue_t &x)
  : statement_t (x.lineno) {}

//-----------------------------------------------------------------------

bool
pub3::continue_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_CONTINUE);
  x->break_statement->lineno = lineno ();
  return true;
}

//-----------------------------------------------------------------------

pub3::call_t::call_t (const xpub3_call_t &x)
  : expr_t (x.lineno),
    _fn (expr_t::alloc (x.fn)),
    _arglist (expr_list_t::alloc (x.args)) {}

//-----------------------------------------------------------------------

ptr<pub3::call_t> pub3::call_t::alloc (const xpub3_call_t &x)
{ return New refcounted<call_t> (x); }

//-----------------------------------------------------------------------

pub3::fndef_t::fndef_t (const xpub3_fndef_t &f)
  :  statement_t (f.lineno),
     _name (f.name),
     _lambda (lambda_t::alloc (f.lambda)) {}

//-----------------------------------------------------------------------

ptr<pub3::fndef_t> pub3::fndef_t::alloc (const xpub3_fndef_t &x)
{ return New refcounted<fndef_t> (x); }

//-----------------------------------------------------------------------

bool
pub3::fndef_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_FNDEF);
  x->fndef->name = _name;
  _lambda->to_xdr (&x->fndef->lambda);
  return true;
}

//-----------------------------------------------------------------------

pub3::lambda_t::lambda_t (const xpub3_lambda_t &l)
  : expr_t (l.lineno),
    _params (xdr_to_idlist (l.params)),
    _body (zone_t::alloc (l.body)) {}
    
//-----------------------------------------------------------------------

ptr<pub3::lambda_t> pub3::lambda_t::alloc (const xpub3_lambda_t &l)
{ return New refcounted<lambda_t> (l); } 

//-----------------------------------------------------------------------

bool
pub3::lambda_t::to_xdr (xpub3_lambda_t *l) const
{
  l->lineno = _lineno;
  idlist_to_xdr (*_params, &l->params);
  l->body.alloc ();
  _body->to_xdr (l->body);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::lambda_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_LAMBDA);
  return to_xdr (x->lambda);
}

//-----------------------------------------------------------------------

bool
pub3::zone_text_t::to_xdr (xpub3_zone_t *z) const
{
  z->set_typ (XPUB3_ZONE_TEXT);
  z->text->lineno = lineno ();
  strip ();
  zstr_to_xdr (_original, &z->text->original_text, Z_BEST_COMPRESSION);
  zstr_to_xdr (_wss, &z->text->wss_text, Z_BEST_COMPRESSION);
  return true;
}

//-----------------------------------------------------------------------

ptr<pub3::zone_text_t> pub3::zone_text_t::alloc (const xpub3_zone_text_t &x)
{ return New refcounted<zone_text_t> (x); }

//-----------------------------------------------------------------------

pub3::zone_text_t::zone_text_t (const xpub3_zone_text_t &x)
  : zone_t (x.lineno),
    _original (xdr_to_zstr (x.original_text)),
    _wss (xdr_to_zstr (x.wss_text)) {}

//-----------------------------------------------------------------------

pub3::zone_pub_t::zone_pub_t (const xpub3_zone_pub_t &z)
  : zone_t (z.lineno)
{
  for (size_t i = 0; i < z.statements.size (); i++) {
    _statements.push_back (statement_t::alloc (z.statements[i])); 
  }
}

//-----------------------------------------------------------------------

ptr<pub3::zone_pub_t> pub3::zone_pub_t::alloc (const xpub3_zone_pub_t &x)
{ return New refcounted<zone_pub_t> (x); }


//-----------------------------------------------------------------------

bool
pub3::zone_pub_t::to_xdr (xpub3_zone_t *x) const
{
  x->set_typ (XPUB3_ZONE_PUB);
  x->zone_pub->lineno = lineno ();
  x->zone_pub->statements.setsize (_statements.size ());
  for (size_t i = 0; i < _statements.size (); i++) {
    _statements[i]->to_xdr (&x->zone_pub->statements[i]);
  }
  return true;
}

//-----------------------------------------------------------------------

pub3::zone_html_t::zone_html_t (const xpub3_zone_html_t &z)
  : zone_container_t (z.lineno),
    _preserve_white_space (z.preserve_white_space)
{
  for (size_t i = 0; i < z.zones.size (); i++) {
    _children.push_back (zone_t::alloc (z.zones[i]));
  }
}

//-----------------------------------------------------------------------

ptr<pub3::zone_html_t> pub3::zone_html_t::alloc (const xpub3_zone_html_t &x)
{ return New refcounted<zone_html_t> (x); }

//-----------------------------------------------------------------------

bool
pub3::zone_html_t::to_xdr (xpub3_zone_t *z) const
{
  z->set_typ (XPUB3_ZONE_HTML);
  z->html->lineno = lineno ();
  z->html->preserve_white_space = _preserve_white_space;
  z->html->zones.setsize (_children.size ());
  for (size_t i = 0; i < _children.size (); i++) {
    _children[i]->to_xdr (&z->html->zones[i]);
  }
  return true;

}

//-----------------------------------------------------------------------

pub3::statement_zone_t::statement_zone_t (const xpub3_statement_zone_t &z)
  : statement_t (z.lineno),
    _zone (zone_t::alloc (z.zone)) {}

//-----------------------------------------------------------------------

bool
pub3::statement_zone_t::to_xdr (xpub3_statement_t *x) const
{
  x->set_typ (XPUB3_STATEMENT_ZONE);
  x->zone->lineno = lineno ();
  _zone->to_xdr (&x->zone->zone);
  return true;
}

//-----------------------------------------------------------------------

pub3::decl_block_t::decl_block_t (const xpub3_decls_t &x)
  : statement_t (x.lineno),
    _bindings (bindlist_t::alloc (x.decls)), 
    _tab (expr_dict_t::alloc (x.decls)) {}

//-----------------------------------------------------------------------


pub3::bindlist_t::bindlist_t (const xpub3_dict_t &x)
  : _lineno (x.lineno)
{
  for (size_t i = 0; i < x.entries.size (); i++) {
    push_back (binding_t (x.entries[i]));
  }
}

//-----------------------------------------------------------------------

ptr<pub3::bindlist_t>
pub3::bindlist_t::alloc (const xpub3_dict_t &x)
{ return New refcounted<bindlist_t> (x);  }

//-----------------------------------------------------------------------

bool
pub3::bindlist_t::to_xdr (xpub3_dict_t *x) const
{
  x->lineno = _lineno;
  x->entries.setsize (size ());
  for (size_t i = 0; i < size (); i++) {
    xpub3_binding_t &b = x->entries[i];
    const binding_t &me = (*this)[i];
    b.key = me.name ();
    expr_to_rpc_ptr (me.expr (), &b.val);
  }
  return true;
}

//-----------------------------------------------------------------------
