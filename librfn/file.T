// -*-c++-*-

#include "okrfn-int.h"
#include "okformat.h"
#include "pub3out.h"

namespace rfn3 {

  //-----------------------------------------------------------------------
  
  ptr<const expr_t> 
  stat_file_t::eval_to_val (eval_t *p, args_t args) const { return NULL; }

  //-----------------------------------------------------------------------

  tamed void
  stat_file_t::pub_to_val (eval_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      str f;
      status_t status;
      ptr<file_t> file;
      str rfn, err;
    }
    if (args->size () != 1) {
      report_error (p, "stat_file() takes 1 argument: a file name");
    } else {
      twait { (*args)[0]->pub_as_str (p, mkevent (f)); } 
      if (!f) {
	report_error (p, "cannot call stat_file() on null");
      }
    }
    if (f) {
      twait {
	p->pub_iface ()->
	  publish_prepare (p, f, &rfn, &err, mkevent (status, file));
      }
      if (file) {
	ptr<expr_dict_t> d = file->metadata ()->to_dict ();
	if (d) { ret = d->copy (); }
      } else if (status.status == XPUB_STATUS_NOENT) {
	/* noop */
      } else if (err) {
	report_error (p, strbuf ("error in stat_file(): %s\n", err.cstr ()));
      } else {
	report_error (p, strbuf ("error in stat_file(): code=%d\n", 
				 int (status.status)));
      }
    }
    if (!ret) ret = expr_null_t::alloc ();
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------
  
  ptr<const expr_t> 
  raw_t::eval_to_val (eval_t *p, args_t args) const { return NULL; }

  //-----------------------------------------------------------------------

  tamed void
  raw_t::pub_to_val (eval_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      str f;
      str dat;
      opts_t opts (P_NOPARSE|P_NOLOCALE);
      zbuf b;
      bool ok;
    }
    if (args->size () != 1) {
      report_error (p, "raw(**) takes 1 argument: a file name");
    } else {
      twait { (*args)[0]->pub_as_str (p, mkevent (f)); } 
      if (!f) {
	report_error (p, "cannot call stat_file() on null");
      }
    }

    if (f) {
      twait { p->pub_iface ()->run (&b, f, mkevent (ok), NULL, opts); }
      if (ok) {
	strbuf tmp; 
	b.output (&tmp); 
	dat = tmp;
      } else{
	report_error (p, strbuf ("failed to pub file '%s'", f.cstr ()));
      }
    }
    ret = expr_str_t::safe_alloc (dat);
    ev->trigger (ret);
  };

  const str raw_t::DOCUMENTATION = R"*(Pubs out a file...

@param {string} filename
@return {string})*";
  //-----------------------------------------------------------------------

};
