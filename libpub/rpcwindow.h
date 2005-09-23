// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2005 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _LIBPUB_RPCWINDOW_H
#define _LIBPUB_RPCWINDOW_H 1

/**
 * Type of callback that the windower calls to actually do an RPC
 * call.  The windower passes the calling code the callback that it
 * should redirect to after the RPC comes back (or fails/timesout)
 */
typedef callback<bool, int, aclnt_cb >::ref do_cb_t;

/**
 * Type of callback that the window calls after the RPC returns
 * so that the caller can get a chance to react to the returned
 * RPC status code.
 */
typedef callback<bool, int, clnt_stat>::ref did_cb_t;

/**
 * main class for the asyncrhonous  RPC windower, used to fire
 * off a batch of RPCs with some primitive flow control so that
 * RPC servers dosn't get slamed, or so that individual packet
 * sizes don't get too big and crash pubd).
 */
class rpc_windower_t {
public:

  /**
   * Constructor for creating a new RPC windower object.  The
   * user should pass in some constants, and also three callbacks.
   *
   * The windower will call the first callback (do_call) when it wants 
   * to fire off an RPC. It will supply do_call with the sequence
   * number of the RPC, and also with a callback that should be passed
   * into aclnt::call.  The do_call callback will return true on success
   * and false on failure.  On failure, the windower's 'abort' flag
   * is set, and no further RPCs are scheduled to go out. 
   *
   * Similarly for did_call, which the windower will call after the RPC 
   * returns, * supplying the callee with the sequence number of the RPC and
   * the error code returned by the RPC.
   *
   * Finally, the 'done' callback is called when the windower completes
   * its task (or aborts early). The windower will give the callee
   * a True on success and a false on failure.
   *
   * @brief create a new windower object, but don't start it
   * @param n the number of calls to make, total
   * @param w maximum window size for RPCs in flight
   * @param do_call callback for firing off an RPC call
   * @param did_call callback after an RPC returns
   * @param done callback for when all RPCs are done or abort signaled.
   */
  rpc_windower_t (int n, int w, do_cb_t do_call, did_cb_t did_call, 
		  cbb_t done) :
    _n_calls (n), _i (0), _n_out (0), _n_done (0), _n_issued (0), 
    _abort (false), _window_sz (w), _do_call_cb (do_call), 
    _did_call_cb (did_call), _done_cb (c) {}


  /**
   * Once initialized, run this windower, and sit back. 
   */
  void run ();

protected:
  void fire_calls ();
  void fire_call ();
  void call_cb (int i, clnt_stat st);
  void done () ;

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
  cbb::ptr _done_cb;      // cb to call when done (true = SUCC, false = FAIL)
};
