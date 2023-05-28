/* SPDX-License-Identifier: Unlicense */
/**
 * @file log.h
 * @brief Logging API for short tests.
 */
#if !defined(hg_Reedkiln_log_h_)
#define hg_Reedkiln_log_h_

#include "reedkiln.h"

#if defined(__cplusplus)
extern "C" {
#endif /*__cplusplus*/


/**
 * @brief Write bytes to the log buffer.
 * @param buffer bytes to write
 * @param count number of bytes to write
 * @return the number of bytes actually written
 */
reedkiln_size reedkiln_log_write(void const* buffer, reedkiln_size count);

#if defined(__cplusplus)
};
#endif /*__cplusplus*/

#if defined(__cplusplus)

#include <streambuf>
#include <ostream>
#include <locale>
#include <cwchar>

namespace reedkiln {
  /**
   * @brief STL stream buffer implementation using Reedkiln's log.
   * @tparam Ch character type
   * @tparam Traits character traits type
   */
  template <typename Ch, typename Traits>
  class cxx_logbuf : public std::basic_streambuf<Ch,Traits> {
  public:
    using typename std::basic_streambuf<Ch,Traits>::char_type;
    using typename std::basic_streambuf<Ch,Traits>::int_type;
  private:
    using cvt_facet = std::codecvt<Ch,char,std::mbstate_t>;
    cvt_facet const* cvt;
    std::mbstate_t mbstate;
    char_type buf[8];
  public:
    /** @brief Default constructor. */
    cxx_logbuf() : cvt() {
      resetp();
    }
    /** @brief Destructor. */
    ~cxx_logbuf() { };
    /**
     * @brief Copy constructor.
     * @param x stream buffer whose locale to imbue
     */
    cxx_logbuf(cxx_logbuf<Ch,Traits> const& x) : cvt() {
      pubimbue(x.getloc());
      resetp();
    }
    /**
     * @brief Copy assignment operator.
     * @param x stream buffer whose locale to imbue
     * @return `*this`
     */
    cxx_logbuf& operator=(cxx_logbuf<Ch,Traits> const& x) {
      pubimbue(x.getloc());
      resetp();
      return *this;
    }
  protected:
    /**
     * @brief `imbue` for `std::streambuf`.
     * @param l locale to use
     */
    void imbue(std::locale const& l) {
#if (defined Reedkiln_UseConstexpr)
      constexpr std::mbstate_t zero = {};
#else
      std::mbstate_t zero = {};
#endif /*Reedkiln_UseConstexpr*/
      if (!std::has_facet<cvt_facet>(l))
        cvt = nullptr;
      else {
        cvt_facet const& f = std::use_facet<cvt_facet>(l);
        cvt = (f.always_noconv() ? nullptr : &f);
      }
      mbstate = zero;
      return;
    }
    /**
     * @brief `overflow` for `std::streambuf`
     * @param ch additional character to add to put area
     * @return zero
     */
    int_type overflow(int_type ch) {
      sync();
      if (ch != Traits::eof()) {
        this->sputc(ch);
      }
      return 0;
    }
    /**
     * @brief `xsputn` for `std::streambuf`
     * @param s bulk data to add to log
     * @param count number of `char_type` units to add
     * @return count on success, less than count on encode error
     * @note Overflow of associated Reedkiln log is ignored.
     */
    std::streamsize xsputn(char_type const* s, std::streamsize count) {
      if ((!cvt) && sizeof(char_type) == sizeof(char)) {
        sync();
        reedkiln_log_write(s, static_cast<std::size_t>(count));
        return count;
      } else {
        std::streamsize i;
        for (i = 0; i < count; ++i) {
          int const put_result = this->sputc(s[i]);
          if (put_result == Traits::eof())
            break;
        }
        return i;
      }
    }
    /**
     * @brief `sync` for `std::streambuf`
     * @return zero on success, negative on encode error
     */
    int sync() {
      char outbuf[32];
      const char_type* from_next = this->pbase();
      const char_type* const from_end = this->pptr();
      char* to_next = outbuf;
      char* const to_end = outbuf + sizeof(outbuf);
      std::codecvt_base::result res;
      if (from_next == from_end)
        return 0;
      else if (!cvt) {
        for (; from_next < from_end; ++from_next, ++to_next) {
          if (to_next == to_end) {
            reedkiln_log_write(outbuf, to_next - outbuf);
            to_next = outbuf;
          }
          *to_next = static_cast<char>(*from_next);
        }
        if (to_next > outbuf)
          reedkiln_log_write(outbuf, to_next - outbuf);
        res = std::codecvt_base::ok;
      } else {
        res = cvt->out(mbstate, from_next, from_end, from_next,
            outbuf, outbuf+sizeof(outbuf), to_next);
        while (res == std::codecvt_base::partial) {
          reedkiln_log_write(outbuf, to_next - outbuf);
          res = cvt->out(mbstate, from_next, from_end, from_next,
                outbuf, outbuf+sizeof(outbuf), to_next);
        }
        if (to_next > outbuf)
          reedkiln_log_write(outbuf, to_next - outbuf);
      }
      resetp();
      return (res == std::codecvt_base::error) ? -1 : 0;
    }
  private:
    void resetp() {
      this->setp(buf,buf+8);
    }
  };

  /**
   * @brief Get the narrow STL stream for the Reedkiln log.
   * @return a reference to a std::ostream
   */
  inline
  std::ostream& cxx_log() {
    using buf_type =
      cxx_logbuf<std::ostream::char_type, std::ostream::traits_type>;
    static buf_type buf;
    static std::ostream os(&buf);
    return os;
  }
  /**
   * @brief Get the wide STL stream for the Reedkiln log.
   * @return a reference to a std::wostream
   */
  inline
  std::wostream& cxx_wlog() {
    using buf_type =
      cxx_logbuf<std::wostream::char_type, std::wostream::traits_type>;
    static buf_type buf;
    static std::wostream os(&buf);
    return os;
  }
};

#endif /*__cplusplus*/

#endif /*hg_Reedkiln_log_h_*/
