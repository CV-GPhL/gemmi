// Copyright 2017 Global Phasing Ltd.
//
// to_str(float|double), handling -D USE_STD_SNPRINTF

#ifndef GEMMI_SPRINTF_HPP_
#define GEMMI_SPRINTF_HPP_

#ifdef USE_STD_SNPRINTF  // for benchmarking and testing only
# include <cstdio>
# define stbsp_snprintf std::snprintf
# define stbsp_sprintf std::sprintf
#else
# ifdef GEMMI_WRITE_IMPLEMENTATION
#  define STB_SPRINTF_IMPLEMENTATION
# endif
//# define STB_SPRINTF_DECORATE(name) gemmi_##name
# include <stb_sprintf.h>
#endif
#include <string>

namespace gemmi {

inline std::string to_str(double d) {
  char buf[24];
  int len = stbsp_sprintf(buf, "%.9g", d);
  return std::string(buf, len > 0 ? len : 0);
}

inline std::string to_str(float d) {
  char buf[16];
  int len = stbsp_sprintf(buf, "%.6g", d);
  return std::string(buf, len > 0 ? len : 0);
}

template<int Prec>
std::string to_str_prec(double d) {
  static_assert(Prec >= 0 && Prec < 7, "unsupported precision");
  char buf[16];
  int len = d > -1e8 && d < 1e8 ? stbsp_sprintf(buf, "%.*f", Prec, d)
                                : stbsp_sprintf(buf, "%g", d);
  return std::string(buf, len > 0 ? len : 0);
}


} // namespace gemmi
#endif
// vim:sw=2:ts=2:et
