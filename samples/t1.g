// -*-c++-*-
/* $Id$ */

#include "ok.h"
#include "cgi.h"
#include "pub.h"
#include <unistd.h>
#include "t1_prot.h"
#include "amysql.h"
#include "form.h"

#define T1_INC_1 "/t1-1.html"
#define T1_INC_2 "/t1-2.html"

class webform_t1a_t : public webform_t {
public:
  webform_t1a_t () : webform_t ("t1a")
  {
    add_email ();
    add_name ();
    add_zip ();
    add_sex ();
    add_yob ();
    add_submit ();
  }
};

class webform_t1b_t : public webform_t {
public:
  webform_t1b_t () : webform_t ("t1b")
  {
    add_int ("id", "DB ID:#", 0, 0xffffffff);
    add_submit ();
  }
};

class oksrvc_t1_t : public oksrvc_t {
public:
  oksrvc_t1_t (int argc, char *argv[]) : oksrvc_t (argc, argv) 
  { 
    db = add_db ("-", T1D_PORT, t1_prog_1); 
  }
  okclnt_t *make_newclnt (ptr<ahttpcon> x);

  void init_publist () 
  { 
    add_pubfile (T1_INC_1); 
    add_pubfile (T1_INC_2); 
  }
  dbcon_t *db;
};


class okclnt_t1_t : public okclnt_t {
public:
  okclnt_t1_t (ptr<ahttpcon> x, oksrvc_t1_t *o) 
    : okclnt_t (x, o), form (), oksrvc_t1 (o) {}
  ~okclnt_t1_t () {}
  dbcon_t *db () { return oksrvc_t1->db; }
  void process ()
  {
    form.set_cgi (cgi);
    form_1b.set_cgi (cgi);

    bool to_out = true;
    if (form_1b.submitted ()) {
      if (form_1b.process ()) {
	db_qry ();
	to_out = false;
      } else {
	err = form_1b.error ();
      }
    } else if (form.submitted ()) {
      if (form.process ()) {
	db_ins ();
	to_out = true;
      } else {
	err = form.error ();
      }
    } 
    if (to_out) 
      myoutput (false);
  }

  void make_user (const webform_t &f)
  {
    u.email = f["email"];
    u.name = f["name"];
    u.zipcode = f["zip"];
    f.lookup ("yob", &u.yob);
    u.sex = str_to_sex (f["sex"]);
  }

  void db_qry () 
  {
    t1_query_arg_t arg (T1_QUERY_ID);
    int64_t id;
    assert (form_1b.lookup ("id", &id));
    *arg.id = id;
    db ()->call (T1_QUERY, &arg, &qres, 
		 wrap (this, &okclnt_t1_t::qry_cb, id));
  }

  void db_ins ()
  {
    make_user (form);
    db ()->call (T1_INSERT, &u, &res, wrap (this, &okclnt_t1_t::ins_cb));
  }

  void qry_cb (int64_t id, clnt_stat e)
  {
    if (e) {
      warn << "RPC error: " << e << "\n";
      form_1b.add_error ("Connection to DB failed");
    } else if (qres.status == ADB_NOT_FOUND) {
      form_1b.add_error (strbuf () << id << ": ID not found");
      form_1b.clear ();
    } else {
      form_1b.add_ivar ("USER", 1)
	.add_ivar ("name", qres.user->name)
	.add_ivar ("zipcode", qres.user->zipcode)
	.add_ivar ("yob", qres.user->yob)
	.add_ivar ("sex", sex_to_str (qres.user->sex));
    }
    err = form_1b.error ();
  }

  void ins_cb (clnt_stat e)
  {
    if (e) {
      warn << "RPC error:" << e << "\n";
      form.add_error ("Connection to DB failed");
    } else if (res.status != ADB_OK) {
      form.add_error ("Database failure in insert");
    } else {
      u.id = *res.id;
    }
    err = form.error ();
    myoutput (true);
  }

  void myoutput (bool f)
  {
    aarr_t a;
    str err;

    if (!err)
      form.clear ();

    form.fill (&a);
    form_1b.fill (&a);

    pub->include (&out, T1_INC_1, P_DEBUG, &a);
    if (err) {
      /*<pub>
	print (out) <<EOF;
<font color="red">
<b>There Were Errors in Your Form Submission</b><br>
@{err}
</font>
<br>
EOF
</pub>*/
    } else if (f) {
      /*<pub>
	print (out) <<EOF;
<b>Successful Submission</b>
<br>
For email=@{u.email}, User ID is id=@{u.id}
<br>
EOF
</pub>*/
    } else {
      /*<pub>
      print (out) <<EOF;
<b>Welcome to this Webform</b>
<br>
EOF
</pub>*/
    }
    pub->include (&out, T1_INC_2, P_DEBUG, &a);
    output (out);
  }
  t1_user_t u;
  t1_insert_res_t res;
  t1_query_res_t qres;
  webform_t1a_t form;
  webform_t1b_t form_1b;
  oksrvc_t1_t *oksrvc_t1;
  str err;
};

okclnt_t 
*oksrvc_t1_t::make_newclnt (ptr<ahttpcon> x) 
{ 
  return New okclnt_t1_t (x, this); 
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_t1_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
