
#include "ezdb_field.h"

namespace ezdb {

//-----------------------------------------------------------------------

  field_t::field_t (const MYSQL_FIELD &f)
    : _name (str (f.name, f.name_length)),
      _org_name (str (f.org_name, f.org_name_length)),
      _table (str (f.table, f.table_length)),
      _org_table (str(f.org_table, f.org_table_length)),
      _db (str (f.db, f.db_length)),
      _catalog (str (f.catalog, f.catalog_length)),
      _def (str(f.def, f.def_length)),
      _length (f.length),
      _max_length (f.max_length),
      _flags (f.flags),
      _decimals (f.decimals),
      _charsetnr (f.charsetnr),
      _type (f.type) {}
  
  //-----------------------------------------------------------------------
  
  field_t::field_t (const amysql_field_t &x)
    : _name (x.name), 
      _org_name (x.org_name),
      _table (x.table),
      _org_table (x.org_table),
      _db (x.db),
      _catalog (x.catalog),
      _def (x.def),
      _length (x.length),
      _max_length (x.max_length),
      _flags (x.flags),
      _decimals (x.decimals),
      _charsetnr (x.charsetnr),
      _type (eft_t (x.type)) {}

  //-----------------------------------------------------------------------
  
  void
  field_t::to_xdr (amysql_field_t *x) const
  {
    x->name = _name;
    x->org_name = _org_name;
    x->table = _table;
    x->org_table = _org_table;
    x->db = _db;
    x->catalog = _catalog;
    x->def = _def;
    x->length = _length;
    x->max_length = _max_length;
    x->flags = _flags;
    x->decimals = _decimals;
    x->charsetnr = _charsetnr;
    x->type = int (_type);
  }

  //-----------------------------------------------------------------------

  void
  fields_t::init (const amysql_fields_t &x)
  {
    reserve (x.size ());
    for (size_t i = 0; i < x.size (); i++) {
      push_back (x[i]);
    }
  }

  //-----------------------------------------------------------------------
};
