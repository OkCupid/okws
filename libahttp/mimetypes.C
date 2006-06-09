
#include "mimetypes.h"

mime_type_map_t::mime_type_map_t (const pval_w_t &a)
{
  static rxx badchar ("[^a-zA-Z0-9]");
  for (size_t i = 0; i < a.size (); i++) {

    // Format is:
    //   ("image/jpeg", "jpe", "jpeg", "jpg")
    //   ("text/html", "html")
    if (a[i].size () >= 2) {

      for (size_t j = 1; j < a[i].size (); j++) {
	if (badchar.search (a[i][j])) {
	  warn << "skipping mime type '" << str (a[i][j]) 
	       << "' due to illegal "
	       << "character in suffix.\n";
	} else {
	  _table.insert (a[i][j], a[i][0]);
	}
      }
    }
  }
}

static str
get_suffix (const str &in)
{
  static rxx x ("\\.([a-zA-Z0-9]+)$");
  return x.search (in) ? x[1] : NULL;
}

  
str
mime_type_map_t::lookup (const str &in, str *sffx_p) const
{
  const str *typ;
  str ret;
  if (in) {
    str sffx = get_suffix (in);
    if (sffx) {
      if ((typ = _table[sffx])) ret = *typ;
      if (sffx_p) *sffx_p = sffx;
    }
  }
  return ret;
}



