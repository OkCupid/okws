

enum okws_fd_typ_t {
  OKWS_PUBD = 1,
  OKWS_LOGD = 2,
  OKWS_SVC_X = 3,
  OKWS_SVC_CTL_X = 4
};

struct okws_svc_descriptor_t {
  string name<256>;
  int pid;
};

union okws_fd_t switch (okws_fd_typ_t fdtyp) {
 case OKWS_SVC_X:
   okws_svc_descriptor_t x;
 case OKWS_SVC_CTL_X:
   okws_svc_descriptor_t ctlx;
 default:
   void;
};
