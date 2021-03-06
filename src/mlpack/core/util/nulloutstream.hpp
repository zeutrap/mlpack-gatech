/**
 * @file nulloutstream.hpp
 * @author Ryan Curtin
 * @author Matthew Amidon
 *
 * Definition of the NullOutStream class.
 */
#ifndef __MLPACK_CORE_UTIL_NULLOUTSTREAM_HPP
#define __MLPACK_CORE_UTIL_NULLOUTSTREAM_HPP

#include <iostream>
#include <streambuf>
#include <string>

namespace mlpack {
namespace util {

/**
 * Used for Log::Debug when not compiled with debugging symbols.  This class
 * does nothing and should be optimized out entirely by the compiler.
 */
class NullOutStream
{
 public:
  /**
   * Does nothing.
   */
  NullOutStream() { }

  /**
   * Does nothing.
   */
  NullOutStream(const NullOutStream& /* other */) { }

  /*
   We use (void) paramName in order to avoid the warning generated by
   -Wextra. For some currently unknown reason, simply deleting the
   parameter name (aka, outperator<<(bool) {...}) causes a compilation
   error (with -Werror off) for only this class.
   */

  //! Does nothing.
  NullOutStream& operator<<(bool val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(short val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(unsigned short val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(int val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(unsigned int val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(long val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(unsigned long val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(float val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(double val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(long double val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(void* val) { (void) val; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(const char* str) { (void) str; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(std::string& str) { (void) str; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(std::streambuf* sb) { (void) sb; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(std::ostream& (*pf) (std::ostream&))
  { (void) pf; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(std::ios& (*pf) (std::ios&)) { (void) pf; return *this; }
  //! Does nothing.
  NullOutStream& operator<<(std::ios_base& (*pf) (std::ios_base&))
  { (void) pf; return *this; }

  //! Does nothing.
  template<typename T>
  NullOutStream& operator<<(T& s)
  { (void) s; return *this; }
};

} // namespace util
} // namespace mlpack

#endif
