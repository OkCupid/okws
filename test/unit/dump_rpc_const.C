
#include "pub.h"
#include "ok.h"
#include "json_rpc.h"


int main(int argc, char *argv[])
{
  json_XDR_dispatch_t::enable ();
  json_fetch_constants_t jfc;
  ptr<pub3::expr_t> x = xdr2json (jfc.constant_set ());
  str s = x->to_str ();
  rpc_constant_set_t set;
  json2xdr (set, x);
  warn << s << "\n";
  return 0;
}
