// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_AHTTP
#define _LIBAHTTP_AHTTP

#include "arpc.h"
#include "suiolite.h"
#include "okconst.h"

#define AHTTP_MAXLINE 1024

//
// memory recycling code hacked in for now....
//
#define RECYCLE_LIMIT 2048
void recycle (suio *s);
void recycle (suiolite *s);
suiolite *suiolite_alloc (int mb, cbv::ptr cb);
suio *suio_alloc ();

struct fdtosend {
  const int fd;
  mutable bool closeit;
  fdtosend (int f, bool c) : fd (f), closeit (c) {}
  ~fdtosend () { if (closeit) close (fd); }
  fdtosend (const fdtosend &f) : fd (f.fd), closeit (f.closeit)
  { f.closeit = false; }
};

class ahttpcon_clone;
class ahttpcon : public virtual refcount 
{
  vec<u_int64_t> syncpts; // sync points

protected:
  vec<fdtosend> fdsendq;

public:
  ahttpcon (int f, sockaddr_in *s = NULL, int mb = SUIOLITE_DEF_BUFLEN,
	    int rcvlmt = -1)
    : fd (f), rcbset (false), wcbset (false), bytes_recv (0), bytes_sent (0),
      eof (false), destroyed (false), out (suio_alloc ()), sin (s),
      recv_limit (rcvlmt < 0 ? int (ok_reqsize_limit) : rcvlmt)
  {
    make_async (fd);
    close_on_exec (fd);
    in = suiolite_alloc (mb, wrap (this, &ahttpcon::spacecb));
    set_remote_ip ();
  }
  bool ateof () const { return eof; }
  inline sockaddr_in *get_sin () const { return sin; }
  inline const str & get_remote_ip () const { return remote_ip; }
  virtual ~ahttpcon ();
  void sendfd (int sfd, bool closeit = true);
  // void stopread ();
  void setrcb (cbi::ptr cb); // cb called when reading regular byte streams
  void seteofcb (cbv::ptr c) { eofcb = c; }
  void clone (ref<ahttpcon_clone> xc);
  void output ();
  void spacecb ();
  void error (int ec);
  void send (const strbuf &b, cbv::ptr cb);
  void sendv (const iovec *iov, int cnt, cbv::ptr cb = NULL);
  void copyv (const iovec *iov, int cnt);
  suiolite *uio () const { return in; }
  u_int get_bytes_sent () const { return bytes_sent; }

  static ptr<ahttpcon> alloc (int fd, sockaddr_in *s = NULL, 
			      int mb = SUIOLITE_DEF_BUFLEN)
  { return New refcounted<ahttpcon> (fd, s, mb); }
  bool closed () const { return fd < 0; }

protected:
  void set_remote_ip ();
  virtual int dowritev (int cnt) { return out->output (fd, cnt); }
  virtual ssize_t doread (int fd);
  virtual void recvd_bytes (int n);
  inline void wrsync ();
  virtual void fail ();
  virtual void fail2 () {} 
  void input ();
  bool enable_selread ();

  int fd;
  cbi::ptr rcb;
  cbv::ptr eofcb;
  bool rcbset, wcbset;
  suiolite *in;
  int bytes_recv, bytes_sent;
  bool eof, destroyed;
  suio *out;
  sockaddr_in *sin;
  str remote_ip;
  int recv_limit;
};

// for parent dispatcher, which will send fd's
class ahttpcon_dispatch : public ahttpcon
{
public:
  ahttpcon_dispatch (int f, int mb = SUIOLITE_DEF_BUFLEN) 
    : ahttpcon (f, NULL, mb) {}
  static ptr<ahttpcon_dispatch> alloc (int f, int mb = SUIOLITE_DEF_BUFLEN)
  { return New refcounted<ahttpcon_dispatch> (f, mb); }
protected:
  virtual int dowritev (int cnt);
};

// for child process listening for a file descriptor to come in
typedef callback<void, ptr<ahttpcon> >::ref listencb_t;
class ahttpcon_listen : public ahttpcon 
{
public:
  ahttpcon_listen (int f) : ahttpcon (f) 
  { 
    // XXX - don't want to stop listening to okd! there should be no
    // channel limit here; setting recv_limit = 0 should achieve this.
    recv_limit = 0; 
  }
  void setlcb (listencb_t c);
  static ptr<ahttpcon_listen> alloc (int f) 
  { return New refcounted<ahttpcon_listen> (f); }
protected:
  virtual ssize_t doread (int fd);
  virtual void fail2 ();
  listencb_t::ptr lcb;
};

// for server process to peek () and then pass of the fd
typedef callback<void, str, int>::ref clonecb_t;
class ahttpcon_clone : public ahttpcon
{
public:
  ahttpcon_clone (int f, sockaddr_in *s = NULL, size_t ml = AHTTP_MAXLINE);
  void setccb (clonecb_t cb);
  int takefd ();
  
  static ptr<ahttpcon_clone> 
  alloc (int f, sockaddr_in *s = NULL, size_t ml = AHTTP_MAXLINE) 
  { return New refcounted<ahttpcon_clone> (f, s, ml); }

protected:
  void recvd_bytes (int n);
  virtual void fail2 ();

private:
  void end_read ();
  str delimit (int n);

  const size_t maxline;
  clonecb_t::ptr ccb;

  bool found;
  int delimit_state;
  int delimit_status;
  char *delimit_start;

  u_int bytes_scanned;
};

ptr<ahttpcon> 
ahttpcon_aspawn (str execpath, cbv::ptr postforkcb, ptr<axprt_unix> *ctlx);

int
ahttpcon_aspawn (str execpath, const vec<str> &arv, cbv::ptr pfcb,
		 int *ctlx);

int
ahttpcon_spawn (str execpath, const vec<str> &avs, 
		cbv::ptr postforkcb, bool async, char *const *env,
		int *ctlx);

extern int ahttpcon_spawn_pid;

bool http_server (listencb_t lcb, int port);


#endif /* _LIBAHTTP_AHTTP */
