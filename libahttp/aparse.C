
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

void
async_dumper_t::dump (size_t len, cbs c)
{
  buf = New mstr (len);
  bp = buf->cstr ();
  endp = bp + len;

  dump_cb = c;
  parse (wrap (this, &async_dumper_t::parse_done_cb));
}

void
async_dumper_t::parse_guts ()
{
  size_t rc = 0;
  while (bp < endp) {
    rc = abuf->dump (bp, endp - bp);
    if (rc < 0)
      break; // EOF
    bp += rc;
  }
  if (bp == endp || rc < 0)
      finish_parse (0);
}
