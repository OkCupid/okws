
#include "rpcwindow.h"

void
rpc_windower_t::done ()
{
  // in case we get recursively deleted from calling cb, we cautiously
  // blank the callback in the structure before calling it.
  cbb_t cb = _done_cb;
  _done_cb = NULL;
  (*cb) (!_abort);
}

void
rpc_windower_t::run ()
{
  if (_n_done == _n_calls) 
    done ();
  else
    fire_calls ();
}

void
rpc_windower_t::fire_calls ()
{
  while (_n_issued < _n_calls  && !_abort && _n_out < _window_sz) {
    fire_call ();
  }
}

void
rpc_windower_t::call_cb (int i, clnt_stat err)
{
  if (! (*_did_call_cb) (i, err)) 
    _abort = true;

  -- _n_out;
  ++ _n_done;

  if (_n_out == 0 && (_n_done == _n_calls || _abort)) {
    done ();
  } else {
    fire_calls ();
  }
}

void
rpc_widower_t::fire_call ()
{
  _n_issued ++;
  _n_out ++;
  int i = _i ++;

  bool res = (*_do_call_cb) (i, wrap (this, &rpc_windower_t::call_cb, i));
  if (!res)
    _abort = true;
}

