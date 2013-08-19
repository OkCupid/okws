
#include "okxml.h"
#include "okxmlobj.h"

const xml_obj_ref_t &
xml_obj_ref_t::set_value (ptr<xml_element_t> e)
{
  if (!_el_ref || !_el_ref->assign_to (e))
    _el_ref = e;
  return (*this);
}

xml_obj_const_t
xml_obj_base_t::operator[] (size_t i) const
{
  ptr<const xml_container_t> c;
  ptr<const xml_element_t> e;
  c = to_xml_container ();
  if (c) e = c->get (i);
  else e = xml_null_t::alloc ();
  return xml_obj_const_t (e);
}

xml_obj_const_t
xml_obj_base_t::operator() (const str &i) const
{
  ptr<const xml_struct_t> s;
  ptr<const xml_element_t> e;
  s = to_xml_struct ();
  if (s) e = s->get (i);
  else e = xml_null_t::alloc ();
  return xml_obj_const_t (e);
}

ptr<xml_container_t>
xml_obj_ref_t::coerce_to_container ()
{
  ptr<xml_container_t> c;
  if (!_el_ref || !(c = _el_ref->to_xml_container ())) {
    _el_ref = New refcounted<xml_array_t> ();
    c = _el_ref->to_xml_container ();
  }
  return c;
}

str
xml_obj_base_t::name () const 
{
  ptr<const xml_member_t> m;
  str r;
  if ((m = el ()->to_xml_member ())) 
    r = m->member_name_str ();
  return r;
}

xml_obj_const_t
xml_obj_base_t::value () const
{
  ptr<const xml_member_t> m;
  ptr<const xml_element_t> e;
  if ((m = el ()->to_xml_member ()) && (e = m->member_value ()))
    return xml_obj_const_t (e);
  else
    return xml_obj_const_t (xml_null_t::alloc ());
}

xml_obj_ref_t
xml_obj_ref_t::value ()
{
  ptr<xml_member_t> m;
  if (_el_ref && (m = _el_ref->to_xml_member ()))
    return xml_obj_ref_t (m->member_value ());
  else
    return xml_obj_t (xml_null_t::alloc ());
}

xml_obj_ref_t 
xml_obj_ref_t::operator[] (size_t i) 
{ 
  return xml_obj_ref_t (coerce_to_container ()->get_r (i));
}

void
xml_obj_ref_t::setsize (size_t i)
{
  coerce_to_container ()->setsize (i);
}

xml_obj_ref_t 
xml_obj_ref_t::operator() (const str &i) 
{ 
  ptr<xml_struct_t> s; 

  // corce object to a struct if not one already
  if (!_el_ref || !(s = _el_ref->to_xml_struct ())) {
    _el_ref = New refcounted<xml_struct_t> ();
    s = _el_ref->to_xml_struct ();
  }
  return xml_obj_ref_t (s->get_r (i));
}

xml_obj_t 
xml_obj_base_t::clone () const
{
  return xml_obj_t (el ()->clone ());
}

void
xml_outreq_t::output (zbuf &b) const
{
  b << "<?xml version='1.0'?>\n";
  xml_obj_t::output (b);
}

bool
xml_inresp_t::is_fault (int *code, str *msg) const
{
  ptr<const xml_element_t> e;
  ptr<const xml_method_response_t> r;
  ptr<const xml_fault_t> f;

  // comment out debug statement
  // warn << "obj: "<< el ()->xml_typename () << "\n"; 

  if (el () && 
      (r = el ()->to_xml_method_response ()) && 
      (e = r->body ()) &&
      (f = e->to_xml_fault ())) {
    xml_obj_const_t o (f);
    *code = o("faultCode");
    *msg = o("faultString");
    return true;
  }
  return false;
}

str
xml_obj_base_t::xml_typename (bool coerce) const
{
  // if coerce, descend until we get to a primitive type.
  return str (el() ? 
	      (coerce ? 
	       el()->xml_typename_coerce () : 
	       el()->xml_typename ()) :
	      "unknown"); 
}


//=======================================================================

//-----------------------------------------------------------------------

xml_gobj_const_t
xml_gobj_base_t::operator[] (size_t i) const
{
  if (!v() || i >= v()->size ()) {
    if (o() && i == 0) {
      return xml_gobj_const_t (o(), NULL);
    } else {
      return xml_gobj_const_t ();
    }
  } else {
    return xml_gobj_const_t ((*v())[i], NULL);
  }
}

ptr<const xml_generic_t> 
xml_gobj_base_t::obj () const
{
  if (o()) return o();
  else if (v() && v()->size ()) return ((*v())[0]);
  else return xml_generic_t::const_alloc_null ();
}

xml_gobj_const_t
xml_gobj_base_t::operator() (const str &k) const
{
  const ptr<vec<ptr<xml_generic_t> > > *v = obj ()->lookup (k);
  if (v) { return xml_gobj_const_t (NULL, *v); }
  else { return xml_gobj_const_t (); }
}

size_t
xml_gobj_base_t::len () const
{
  if (!v()) return 0;
  else return v()->size ();
}

//-----------------------------------------------------------------------

xml_gobj_const_t::xml_gobj_const_t (ptr<const xml_element_t> x)
  : _obj (x->to_xml_generic () \
	  ? x->to_xml_generic () 
	  : xml_generic_t::const_alloc_null ()) {}

//-----------------------------------------------------------------------

ptr<xml_generic_t> 
xml_gobj_t::obj () 
{
  if (_obj) return _obj;
  else if (_v && _v->size ()) return ((*_v)[0]);
  else {
    _obj = xml_generic_t::alloc_null();
    return _obj;
  }
}

xml_gobj_t
xml_gobj_t::operator[] (size_t i)
{
  ptr<xml_generic_t> r;
  if ((!_v || _v->size () == 0) && _obj && i == 0) {
    r = _obj;
  } else {
    extend_vec (i+1);
    r = (*_v)[i];
  }
  return xml_gobj_t (r, NULL);
}

xml_gobj_t
xml_gobj_t::operator() (const str &k, int n_atts, ...) 
{
  vec<str> attrs;
  va_list al;
  va_start(al, n_atts);
  for (int i = 0; i < n_atts; ++i) {
      const char* s = va_arg(al, char*);
      attrs.push_back(s);
  }
  va_end(al);

  const ptr<vec<ptr<xml_generic_t> > > *v = obj ()->lookup (k);
  if (v) { 
    return xml_gobj_t (NULL, *v); 
  } else { 
    ptr<xml_generic_t> g = New refcounted<xml_generic_t> (k.cstr(), attrs);
    obj ()->add (g);
    return xml_gobj_t (g);
  }
}

void
xml_gobj_t::extend_vec (size_t i)
{
  if (!_v)
    _v = New refcounted<vec<ptr<xml_generic_t> > > ();
  if (_v->size () < i) 
    _v->setsize (i);
}

xml_gobj_t::xml_gobj_t (ptr<xml_element_t> x)
  : _obj (x->to_xml_generic () \
	  ? x->to_xml_generic () 
	  : xml_generic_t::alloc_null ()) {}


//=======================================================================

void
xml_obj_const_t::to_pub3 (pub3::obj_t *o) const
{
  if (_el) {
    _el->to_pub3 (o);
  }
}

//=======================================================================

xml_obj_t
xml_obj_t::from_pub3 (ptr<const pub3::expr_t> in)
{
  ptr<xml_struct_t> st = New refcounted<xml_struct_t> ();
  xml_obj_t xd (st);
  if (!in) return xd;

  ptr<const pub3::expr_dict_t> d;
  ptr<const pub3::expr_list_t> l;
  int64_t t;
  str s;

  if ((d = in->to_dict ())) {
    pub3::bindtab_t::const_iterator_t it (*d);
    const str *key;
    ptr<pub3::expr_t> x;
    while ((key = it.next (&x))) {
      xml_obj_t tmp = from_pub3 (x);
      st->put (*key, tmp.el ());
    }
  } else if ((l = in->to_list ())) {
    ptr<xml_array_t> a = New refcounted<xml_array_t> ();
    for (size_t i = 0; i < l->size (); i++) {
      xml_obj_t tmp = from_pub3 ((*l)[i]);
      a->put (i, tmp.el ());
    }
    return xml_obj_t (a);
  } else if ((in->to_int (&t))) {
    return xml_obj_t (New refcounted<xml_int_t> (t)); 
  } else if ((s = in->to_str (false))) {
    return xml_obj_t (New refcounted<xml_str_t> (s));
  } 

  return xd;
}

//=======================================================================
