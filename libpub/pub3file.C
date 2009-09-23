#include "pub3file.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  ptr<file_t> file_t::alloc (ptr<metadata_t> m, ptr<zone_t> z)
  { return New refcounted<file_t> (m, z); }

  //-----------------------------------------------------------------------


};
