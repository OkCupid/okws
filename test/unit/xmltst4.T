// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "okxmlgeneric.h"
#include "okxmlobj.h"
#include "tame.h"

static void usage (void)
{
  warnx << "usage: " << progname << "\n";
  exit (1);
}

void
explore (ptr<xml_generic_t> top)
{
  xml_gobj_t o (top);
  xml_gobj_t okt = o("OkTest");
  warn << "OkTest.AboutTheAuthor.Description=" << 
    o("OkTest")("AboutTheAuthor")("Description").data ().trim () << "\n";

  warn << "OkTest.Title=" << o("OkTest")("Title") << "\n";
  o("OkTest")("Title")  = "Doggy Doggy Time";
  warn << "OkTest.Title=" << o("OkTest")("Title") << "\n";
  o("OkTest")("Title")  = 200;
  warn << "OkTest.Title=" << o("OkTest")("Title") << "\n";


  xml_gobj_t var = okt("Var");
  for (size_t i = 0; i < var.len(); i++) {
    warn << "OkTest.Var[" << i << "].Name=" << var[i]("Name").data ().trim ()
	 << "\n";
  }
  warn << "->Var the First=" << var("Name").data ().trim () << "\n";
  warn << "->Var the First=" << var("Name")[0].data ().trim () << "\n";
  
  xml_gobj_key_iterator_t ki (okt);
  str s;
  warn << "OkTest Keys:\n";
  xml_gobj_t o2;
  while ((s = ki.next (&o2))) {
    warn << " " << s << "\n";
  }
  xml_gobj_t citems= okt("Contents")("ContentItem");

  int i = citems[0].attribute("Id");
  warn << "OkTest.Contents.ContentItem[0].attribute(\"id\")=" << i << "\n";

  str k;
  scalar_obj_t so;
  xml_attribute_iterator_t ai (citems[1].attributes ());
  while ((k = ai.next (&so))) {
    warn << "  Attribute: " << k << " -> " << so.trim () << "\n";
  }

  xml_gobj_t tags = okt("Tags")("Tag");
  for (size_t i = 0; i < tags.len (); i++) {
    str s = tags[i].data();
    warn << "  Tag: " << s << "\n";
  }
}

tamed static void
main_T ()
{
  tvars {
    holdvar abuf_pipe_t src (ain);
    xml_req_parser_generic_t xrp (&src);
    int res;
    ptr<xml_generic_t> top;
  }

  xrp.init ();
  twait { xrp.parse (mkevent (res)); }
  if (res == HTTP_OK) {
    top = xrp.top_level_g ();
    explore (top);
  } else {
    assert (xrp.errcode () != XML_PARSE_OK);
    warn << "XML parse error: " << xrp.errmsg () << "\n";
  }
  exit (0);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 1) 
    usage ();

  main_T ();
  amain ();

}
