/**
 * @file range.hpp
 *
 * Definition of the Range class, which represents a simple range with a lower
 * and upper bound.
 */
#ifndef __MLPACK_CORE_MATH_RANGE_HPP
#define __MLPACK_CORE_MATH_RANGE_HPP

namespace mlpack {
namespace math {

/**
 * Simple real-valued range.  It contains an upper and lower bound.
 */
class Range
{
 private:
  double lo; /// The lower bound.
  double hi; /// The upper bound.

 public:
  /** Initialize to an empty set (where lo > hi). */
  Range();

  /***
   * Initialize a range to enclose only the given point (lo = point, hi =
   * point).
   *
   * @param point Point that this range will enclose.
   */
  Range(const double point);

  /**
   * Initializes to specified range.
   *
   * @param lo Lower bound of the range.
   * @param hi Upper bound of the range.
   */
  Range(const double lo, const double hi);

  //! Get the lower bound.
  const double& Lo() const { return lo; }
  //! Modify the lower bound.
  double& Lo() { return lo; }

  //! Get the upper bound.
  const double& Hi() const { return hi; }
  //! Modify the upper bound.
  double& Hi() { return hi; }

  /**
   * Gets the span of the range (hi - lo).
   */
  double Width() const;

  /**
   * Gets the midpoint of this range.
   */
  double Mid() const;

  /**
   * Expands this range to include another range.
   *
   * @param rhs Range to include.
   */
  Range& operator|=(const Range& rhs);

  /**
   * Expands this range to include another range.
   *
   * @param rhs Range to include.
   */
  Range operator|(const Range& rhs) const;

  /**
   * Shrinks this range to be the overlap with another range; this makes an
   * empty set if there is no overlap.
   *
   * @param rhs Other range.
   */
  Range& operator&=(const Range& rhs);

  /**
   * Shrinks this range to be the overlap with another range; this makes an
   * empty set if there is no overlap.
   *
   * @param rhs Other range.
   */
  Range operator&(const Range& rhs) const;

  /**
   * Scale the bounds by the given double.
   *
   * @param d Scaling factor.
   */
  Range& operator*=(const double d);

  /**
   * Scale the bounds by the given double.
   *
   * @param d Scaling factor.
   */
  Range operator*(const double d) const;

  /**
   * Scale the bounds by the given double.
   *
   * @param d Scaling factor.
   */
  friend Range operator*(const double d, const Range& r); // Symmetric case.

  /**
   * Compare with another range for strict equality.
   *
   * @param rhs Other range.
   */
  bool operator==(const Range& rhs) const;

  /**
   * Compare with another range for strict equality.
   *
   * @param rhs Other range.
   */
  bool operator!=(const Range& rhs) const;

  /**
   * Compare with another range.  For Range objects x and y, x < y means that x
   * is strictly less than y and does not overlap at all.
   *
   * @param rhs Other range.
   */
  bool operator<(const Range& rhs) const;

  /**
   * Compare with another range.  For Range objects x and y, x < y means that x
   * is strictly less than y and does not overlap at all.
   *
   * @param rhs Other range.
   */
  bool operator>(const Range& rhs) const;

  /**
   * Determines if a point is contained within the range.
   *
   * @param d Point to check.
   */
  bool Contains(const double d) const;

  /**
   * Determines if another range overlaps with this one.
   *
   * @param r Other range.
   *
   * @return true if ranges overlap at all.
   */
  bool Contains(const Range& r) const;

};

}; // namespace math
}; // namespace mlpack

#endif // __MLPACK_CORE_MATH_RANGE_HPP
