
#include "mimetypes.h"

mime_type_map_t::mime_type_map_t (const pval_w_t &a)
{
  static rxx badchar ("[^a-zA-Z0-9]");
  for (size_t i = 0; i < a.size (); i++) {
    if (a[i].size () == 2) {
      if (badchar.search (a[i][0])) {
	warn << "skipping mime type '" << str (a[i][0]) << "' due to illegal "
	     << "character in suffix.\n";
      } else {
	warn << "inserting (" << str (a[i][0]) << " => " 
	     << str (a[i][1]) << "\n";
	_table.insert (a[i][0], a[i][1]);
	_suffixes.push_back (a[i][0]);
      }
    }
  }
  str l = join ("|", _suffixes);
  strbuf b ("\\.(%s)$", l.cstr ());
  str r = b;
  _matcher = New rxx (r, "i");
}
  
str
mime_type_map_t::lookup (const str &in, str *sffx_p) const
{
  const str *typ;
  str ret;
  if (_matcher->search (in)) {
    const str &sffx = (*_matcher)[1];
    if (sffx_p) *sffx_p = sffx;
    if ((typ = _table[sffx])) ret = *typ;
  }
  return ret;
}



