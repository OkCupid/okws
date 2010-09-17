
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
    JSON_creator_t creator;
    XML_RPC_obj_t *obj = &creator;
    rpc_traverse (obj, aa);
    warn << "aa: " << creator.result()->to_str () << "\n";
  } while (0);

  do {
    JSON_creator_t creator;
    XML_RPC_obj_t *obj = &creator;
    rpc_traverse (obj, ww);
    warn << "ww: " << creator.result()->to_str () << "\n";
  } while (0);

}
