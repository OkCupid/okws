
#include "aparse.h"

void
async_parser_t::parse (cbi::ptr c)
{
  assert (!parsing);
  pcb = c;
  // this call might set dataready
  abuf->init (wrap (this, &async_parser_t::can_read_cb));
  parsing = true;
  if (dataready)
    parse_guts ();
}

void 
async_parser_t::resume ()
{
  parsing = false;
  dataready = false;
  abuf->init (wrap (this, &async_parser_t::can_read_cb));
  parsing = true;
  if (dataready)
    parse_guts ();
}

void
async_parser_t::can_read_cb ()
{
  abuf->can_read ();
  if (parsing)
    parse_guts ();
  else
    dataready = true;
}

void
async_parser_t::cancel ()
{
  if (abuf)
    abuf->cancel ();
  pcb = NULL;
}

void
async_parser_t::finish_parse (int r)
{
  parsing = false;
  if (pcb)
    (*pcb) (r);
}


