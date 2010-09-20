
#include "async.h"
#include "arpc.h"
#include "pub3.h"
#include "okws_sfs.h"
#include "xdr_as_json.h"

#define ENABLE_XML_XDR
#include "tstprot.h"

int 
main (int argc, char *argv[])
{
  yy_t yy[10];
  yy[0].set_xx (XXA);
  yy[0].a->x = 4344;
  yy[0].a->y = "y Y yyy YY yyy YYY yyy YYY yyy YYY yyy YY";
  yy[1].set_xx (XXB);
  *yy[1].x = 302;
  yy[2].set_xx (XXC);
  *yy[2].z = 9993;

  ww_t ww;
  ww.z = "zZ Zzz Zz z ZZ zzZ zz Z";
  ww.v.push_back (yy[0]);
  ww.v.push_back (yy[1]);
  ww.a[0] = yy[2];
  ww.a[1] = yy[1];

  aa_t aa;
  aa.p2.alloc ();
  *aa.p2 = yy[0];
  

  do {
    ptr<pub3::expr_t> x = xdr2json (aa);
    warn << "aa: " << x->to_str () << "\n";
  } while (0);

  do {
    ptr<pub3::expr_t> x = xdr2json (ww);
    warn << "ww: " << x->to_str () << "\n";
  } while (0);

  do {
    ptr<pub3::expr_t> x = xdr2json (ww);
    ww_t ww_new;
    json2xdr (ww_new, x);
    ptr<pub3::expr_t> x2 = xdr2json (ww_new);
    warn << "ww'': " << x2->to_str () << "\n";
  } while (0);
}
