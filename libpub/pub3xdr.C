
#include "pub3.h"

// XDR functions for pub3 objects

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
  : statement_t (x.lineno),
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

ptr<pub3::zone_t>
pub3::zone_t::alloc (const xpub3_zone_t &z)
{
  ptr<pub3::zone_t> r;
  switch (z.typ) {
  case XPUB3_ZONE_HTML:
    r = New refcounted<zone_html_t> (*z.html);
    break;
  case XPUB3_ZONE_TEXT:
    r = New refcounted<zone_text_t> (*z.text);
    break;
  case XPUB3_ZONE_INLINE_EXPR:
    r = New refcounted<zone_inline_expr_t> (*z.zone_inline);
    break;
  case XPUB3_ZONE_PUB:
    r = New refcounted<zone_pub_t> (*z.zone_pub);
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
  case XPUB3_EXPR_FN:
    r = pub3::rfn_factory_t::get ()->alloc (*x.fn);
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

ptr<pub3::runtime_fn_t>
pub3::rfn_factory_t::alloc (const xpub3_fn_t &x)
{
  return alloc (x.name, expr_list_t::alloc (x.args), x.lineno);
}

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
pub3::runtime_fn_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_FN);
  x->fn->lineno = _lineno;
  x->fn->name = name ();
  if (args ()) {
    args ()->to_xdr (&x->fn->args);
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
  x->lineno = lineno ();

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

bool
pub3::expr_varref_or_rfn_t::to_xdr (xpub3_expr_t *x) const
{
  ptr<const expr_t> f = get_rfn ();
  bool ret;
  if (f) {
    ret = f->to_xdr (x);
  } else {
    ret = expr_varref_t::to_xdr (x);
  }
  return x;
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

ptr<case_list_t> pub3::case_list_t::alloc (const xpub3_cases_t &x)
{ return New refcounted<case_list_t> (x); } 

//-----------------------------------------------------------------------

bool
pub3::case_list_t::to_xdr (xpub3_cases_t *x)
{
  x->setsize (size ());
  for (size_t i = 0; i < size (); i++) {
    (*this)[i]->to_xdr (&(*x)[i]);
  }
  return true;
}

//-----------------------------------------------------------------------

pub3::switch_t::switch_t (const xpub3_switch_t &x)
  : pfile_func_t (x.lineno),
    _key (expr_t::alloc (x.key)),
    _cases (case_list_t::alloc (x.cases))
{
  populate_cases ();
}

//-----------------------------------------------------------------------

bool
pub3::switch_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB3_SWITCH);
  x->switch_statement->lineno = lineno;
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
