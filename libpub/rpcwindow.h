

typedef callback<bool, int, aclnt_cb >::ref do_cb_t;
typedef callback<bool, int, clnt_stat>::ref did_cb_t;
class rpc_windower_t {
public:
  rpc_windower_t (int n, int w, do_cb_t do_call, did_cb_t did_call, 
		  cbb_t done) :
    _n_calls (n), _i (0), _n_out (0), _n_done (0), _n_issued (0), 
    _abort (false), _window_sz (w), _do_call_cb (do_call), 
    _did_call_cb (did_call), _done_cb (c) {}


  void run ();

protected:
  void fire_calls ();
  void fire_call ();
  void call_cb (int i, clnt_stat st);
  void done () { (*_done_cb) (!_abort); }

private:

  int _n_calls;       // # of calls to do
  int _i;             // current call
  int _n_out;         // # of calls outstanding
  int _n_done;        // # of calls finished
  int _n_issued;      // # of calls issued
  bool _abort;        // true if we should abort before we are done
  int _window_sz;     // # outstanding to allow

  do_cb_t _do_call_cb;    // call this to DO a call
  did_cb_t _did_call_cb;  // signal that a call was done
  cbb _done_cb;           // cb to call when done (true = SUCC, false = FAIL)
};
