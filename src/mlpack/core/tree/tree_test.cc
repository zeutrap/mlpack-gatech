/**
 * @file uselapack_test.cc
 *
 * Tests for LAPACK integration.
 */

#include "bounds.h"
#include "spacetree.h"
#include <mlpack/core/kernels/lmetric.h>

#define BOOST_TEST_MODULE Tree_Test
#include <boost/test/unit_test.hpp>

using namespace mlpack;
using namespace mlpack::tree;
using namespace mlpack::kernel;
using namespace mlpack::bound;

/***
 * Ensure that a bound, by default, is empty and has no dimensionality.
 */
BOOST_AUTO_TEST_CASE(HRectBoundEmptyConstructor) {
  HRectBound<2> b;

  BOOST_REQUIRE_EQUAL(b.dim(), 0);
}

/***
 * Ensure that when we specify the dimensionality in the constructor, it is
 * correct, and the bounds are all the empty set.
 */
BOOST_AUTO_TEST_CASE(HRectBoundDimConstructor) {
  HRectBound<2> b(2); // We'll do this with 2 and 5 dimensions.

  BOOST_REQUIRE_EQUAL(b.dim(), 2);
  BOOST_REQUIRE_SMALL(b[0].width(), 1e-5);
  BOOST_REQUIRE_SMALL(b[1].width(), 1e-5);

  b = HRectBound<2>(5);

  BOOST_REQUIRE_EQUAL(b.dim(), 5);
  BOOST_REQUIRE_SMALL(b[0].width(), 1e-5);
  BOOST_REQUIRE_SMALL(b[1].width(), 1e-5);
  BOOST_REQUIRE_SMALL(b[2].width(), 1e-5);
  BOOST_REQUIRE_SMALL(b[3].width(), 1e-5);
  BOOST_REQUIRE_SMALL(b[4].width(), 1e-5);
}

/***
 * Test the copy constructor.
 */
BOOST_AUTO_TEST_CASE(HRectBoundCopyConstructor) {
  HRectBound<2> b(2);
  b[0] = Range(0.0, 2.0);
  b[1] = Range(2.0, 3.0);

  HRectBound<2> c(b);

  BOOST_REQUIRE_EQUAL(c.dim(), 2);
  BOOST_REQUIRE_SMALL(c[0].lo, 1e-5);
  BOOST_REQUIRE_CLOSE(c[0].hi, 2.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c[1].lo, 2.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c[1].hi, 3.0, 1e-5);
}

/***
 * Test the assignment operator.
 */
BOOST_AUTO_TEST_CASE(HRectBoundAssignmentOperator) {
  HRectBound<2> b(2);
  b[0] = Range(0.0, 2.0);
  b[1] = Range(2.0, 3.0);

  HRectBound<2> c(4);

  c = b;

  BOOST_REQUIRE_EQUAL(c.dim(), 2);
  BOOST_REQUIRE_SMALL(c[0].lo, 1e-5);
  BOOST_REQUIRE_CLOSE(c[0].hi, 2.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c[1].lo, 2.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c[1].hi, 3.0, 1e-5);
}

/***
 * Test that clearing the dimensions resets the bound to empty.
 */
BOOST_AUTO_TEST_CASE(HRectBoundClear) {
  HRectBound<2> b(2); // We'll do this with two dimensions only.

  b[0] = Range(0.0, 2.0);
  b[1] = Range(2.0, 4.0);

  // Now we just need to make sure that we clear the range.
  b.Clear();

  BOOST_REQUIRE_SMALL(b[0].width(), 1e-5);
  BOOST_REQUIRE_SMALL(b[1].width(), 1e-5);
}

/***
 * Ensure that we get the correct centroid for our bound.
 */
BOOST_AUTO_TEST_CASE(HRectBoundCentroid) {
  // Create a simple 3-dimensional bound.
  HRectBound<2> b(3);

  b[0] = Range(0.0, 5.0);
  b[1] = Range(-2.0, -1.0);
  b[2] = Range(-10.0, 50.0);

  arma::vec centroid;

  b.Centroid(centroid);

  BOOST_REQUIRE_EQUAL(centroid.n_elem, 3);
  BOOST_REQUIRE_CLOSE(centroid[0], 2.5, 1e-5);
  BOOST_REQUIRE_CLOSE(centroid[1], -1.5, 1e-5);
  BOOST_REQUIRE_CLOSE(centroid[2], 20.0, 1e-5);
}

/***
 * Ensure that we calculate the correct minimum distance between a point and a
 * bound.
 */
BOOST_AUTO_TEST_CASE(HRectBoundMinDistancePoint) {
  // We'll do the calculation in five dimensions, and we'll use three cases for
  // the point: point is outside the bound; point is on the edge of the bound;
  // point is inside the bound.  In the latter two cases, the distance should be
  // zero.
  HRectBound<2> b(5);

  b[0] = Range(0.0, 2.0);
  b[1] = Range(1.0, 5.0);
  b[2] = Range(-2.0, 2.0);
  b[3] = Range(-5.0, -2.0);
  b[4] = Range(1.0, 2.0);

  arma::vec point = "-2.0 0.0 10.0 3.0 3.0";

  // This will be the Euclidean squared distance.
  BOOST_REQUIRE_CLOSE(b.MinDistance(point), 95.0, 1e-5);

  point = "2.0 5.0 2.0 -5.0 1.0";

  BOOST_REQUIRE_SMALL(b.MinDistance(point), 1e-5);

  point = "1.0 2.0 0.0 -2.0 1.5";

  BOOST_REQUIRE_SMALL(b.MinDistance(point), 1e-5);
}

/***
 * Ensure that we calculate the correct minimum distance between a bound and
 * another bound.
 */
BOOST_AUTO_TEST_CASE(HRectBoundMinDistanceBound) {
  // We'll do the calculation in five dimensions, and we can use six cases.
  // The other bound is completely outside the bound; the other bound is on the
  // edge of the bound; the other bound partially overlaps the bound; the other
  // bound fully overlaps the bound; the other bound is entirely inside the
  // bound; the other bound entirely envelops the bound.
  HRectBound<2> b(5);

  b[0] = Range(0.0, 2.0);
  b[1] = Range(1.0, 5.0);
  b[2] = Range(-2.0, 2.0);
  b[3] = Range(-5.0, -2.0);
  b[4] = Range(1.0, 2.0);

  HRectBound<2> c(5);

  // The other bound is completely outside the bound.
  c[0] = Range(-5.0, -2.0);
  c[1] = Range(6.0, 7.0);
  c[2] = Range(-2.0, 2.0);
  c[3] = Range(2.0, 5.0);
  c[4] = Range(3.0, 4.0);

  BOOST_REQUIRE_CLOSE(b.MinDistance(c), 22.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c.MinDistance(b), 22.0, 1e-5);

  // The other bound is on the edge of the bound.
  c[0] = Range(-2.0, 0.0);
  c[1] = Range(0.0, 1.0);
  c[2] = Range(-3.0, -2.0);
  c[3] = Range(-10.0, -5.0);
  c[4] = Range(2.0, 3.0);

  BOOST_REQUIRE_SMALL(b.MinDistance(c), 1e-5);
  BOOST_REQUIRE_SMALL(c.MinDistance(b), 1e-5);

  // The other bound partially overlaps the bound.
  c[0] = Range(-2.0, 1.0);
  c[1] = Range(0.0, 2.0);
  c[2] = Range(-2.0, 2.0);
  c[3] = Range(-8.0, -4.0);
  c[4] = Range(0.0, 4.0);

  BOOST_REQUIRE_SMALL(b.MinDistance(c), 1e-5);
  BOOST_REQUIRE_SMALL(c.MinDistance(b), 1e-5);

  // The other bound fully overlaps the bound.
  BOOST_REQUIRE_SMALL(b.MinDistance(b), 1e-5);
  BOOST_REQUIRE_SMALL(c.MinDistance(c), 1e-5);

  // The other bound is entirely inside the bound / the other bound entirely
  // envelops the bound.
  c[0] = Range(-1.0, 3.0);
  c[1] = Range(0.0, 6.0);
  c[2] = Range(-3.0, 3.0);
  c[3] = Range(-7.0, 0.0);
  c[4] = Range(0.0, 5.0);

  BOOST_REQUIRE_SMALL(b.MinDistance(c), 1e-5);
  BOOST_REQUIRE_SMALL(c.MinDistance(b), 1e-5);
}

/***
 * Ensure that we calculate the correct maximum distance between a bound and a
 * point.  This uses the same test cases as the MinDistance test.
 */
BOOST_AUTO_TEST_CASE(HRectBoundMaxDistancePoint) {
  // We'll do the calculation in five dimensions, and we'll use three cases for
  // the point: point is outside the bound; point is on the edge of the bound;
  // point is inside the bound.  In the latter two cases, the distance should be
  // zero.
  HRectBound<2> b(5);

  b[0] = Range(0.0, 2.0);
  b[1] = Range(1.0, 5.0);
  b[2] = Range(-2.0, 2.0);
  b[3] = Range(-5.0, -2.0);
  b[4] = Range(1.0, 2.0);

  arma::vec point = "-2.0 0.0 10.0 3.0 3.0";

  // This will be the Euclidean squared distance.
  BOOST_REQUIRE_CLOSE(b.MaxDistance(point), 253.0, 1e-5);

  point = "2.0 5.0 2.0 -5.0 1.0";

  BOOST_REQUIRE_CLOSE(b.MaxDistance(point), 46.0, 1e-5);

  point = "1.0 2.0 0.0 -2.0 1.5";

  BOOST_REQUIRE_CLOSE(b.MaxDistance(point), 23.25, 1e-5);
}

/***
 * Ensure that we calculate the correct maximum distance between a bound and
 * another bound.  This uses the same test cases as the MinDistance test.
 */
BOOST_AUTO_TEST_CASE(HRectBoundMaxDistanceBound) {
  // We'll do the calculation in five dimensions, and we can use six cases.
  // The other bound is completely outside the bound; the other bound is on the
  // edge of the bound; the other bound partially overlaps the bound; the other
  // bound fully overlaps the bound; the other bound is entirely inside the
  // bound; the other bound entirely envelops the bound.
  HRectBound<2> b(5);

  b[0] = Range(0.0, 2.0);
  b[1] = Range(1.0, 5.0);
  b[2] = Range(-2.0, 2.0);
  b[3] = Range(-5.0, -2.0);
  b[4] = Range(1.0, 2.0);

  HRectBound<2> c(5);

  // The other bound is completely outside the bound.
  c[0] = Range(-5.0, -2.0);
  c[1] = Range(6.0, 7.0);
  c[2] = Range(-2.0, 2.0);
  c[3] = Range(2.0, 5.0);
  c[4] = Range(3.0, 4.0);

  BOOST_REQUIRE_CLOSE(b.MaxDistance(c), 210.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c.MaxDistance(b), 210.0, 1e-5);

  // The other bound is on the edge of the bound.
  c[0] = Range(-2.0, 0.0);
  c[1] = Range(0.0, 1.0);
  c[2] = Range(-3.0, -2.0);
  c[3] = Range(-10.0, -5.0);
  c[4] = Range(2.0, 3.0);

  BOOST_REQUIRE_CLOSE(b.MaxDistance(c), 134.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c.MaxDistance(b), 134.0, 1e-5);

  // The other bound partially overlaps the bound.
  c[0] = Range(-2.0, 1.0);
  c[1] = Range(0.0, 2.0);
  c[2] = Range(-2.0, 2.0);
  c[3] = Range(-8.0, -4.0);
  c[4] = Range(0.0, 4.0);

  BOOST_REQUIRE_CLOSE(b.MaxDistance(c), 102.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c.MaxDistance(b), 102.0, 1e-5);

  // The other bound fully overlaps the bound.
  BOOST_REQUIRE_CLOSE(b.MaxDistance(b), 46.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c.MaxDistance(c), 61.0, 1e-5);

  // The other bound is entirely inside the bound / the other bound entirely
  // envelops the bound.
  c[0] = Range(-1.0, 3.0);
  c[1] = Range(0.0, 6.0);
  c[2] = Range(-3.0, 3.0);
  c[3] = Range(-7.0, 0.0);
  c[4] = Range(0.0, 5.0);

  BOOST_REQUIRE_CLOSE(b.MaxDistance(c), 100.0, 1e-5);
  BOOST_REQUIRE_CLOSE(c.MaxDistance(b), 100.0, 1e-5);

  // One last additional case.  If the bound encloses only one point, the
  // maximum distance between it and itself is 0.
  HRectBound<2> d(2);

  d[0] = Range(2.0, 2.0);
  d[1] = Range(3.0, 3.0);

  BOOST_REQUIRE_SMALL(d.MaxDistance(d), 1e-5);
}

/***
 * Ensure that the ranges returned by RangeDistance() are equal to the minimum
 * and maximum distance.  We will perform this test by creating random bounds
 * and comparing the behavior to MinDistance() and MaxDistance() -- so this test
 * is assuming that those passed and operate correctly.
 */
BOOST_AUTO_TEST_CASE(HRectBoundRangeDistanceBound) {
  srand(time(NULL));

  for (int i = 0; i < 50; i++) {
    size_t dim = rand() % 20;

    HRectBound<2> a(dim);
    HRectBound<2> b(dim);

    // We will set the low randomly and the width randomly for each dimension of
    // each bound.
    arma::vec lo_a(dim);
    arma::vec width_a(dim);

    lo_a.randu();
    width_a.randu();

    arma::vec lo_b(dim);
    arma::vec width_b(dim);

    lo_b.randu();
    width_b.randu();

    for (size_t j = 0; j < dim; j++) {
      a[j] = Range(lo_a[j], lo_a[j] + width_a[j]);
      b[j] = Range(lo_b[j], lo_b[j] + width_b[j]);
    }

    // Now ensure that MinDistance and MaxDistance report the same.
    Range r = a.RangeDistance(b);
    Range s = b.RangeDistance(a);

    BOOST_REQUIRE_CLOSE(r.lo, s.lo, 1e-5);
    BOOST_REQUIRE_CLOSE(r.hi, s.hi, 1e-5);

    BOOST_REQUIRE_CLOSE(r.lo, a.MinDistance(b), 1e-5);
    BOOST_REQUIRE_CLOSE(r.hi, a.MaxDistance(b), 1e-5);

    BOOST_REQUIRE_CLOSE(s.lo, b.MinDistance(a), 1e-5);
    BOOST_REQUIRE_CLOSE(s.hi, b.MaxDistance(a), 1e-5);
  }
}

/***
 * Ensure that the ranges returned by RangeDistance() are equal to the minimum
 * and maximum distance.  We will perform this test by creating random bounds
 * and comparing the bheavior to MinDistance() and MaxDistance() -- so this test
 * is assuming that those passed and operate correctly.  This is for the
 * bound-to-point case.
 */
BOOST_AUTO_TEST_CASE(HRectBoundRangeDistancePoint) {
  srand(time(NULL));

  for (int i = 0; i < 20; i++) {
    size_t dim = rand() % 20;

    HRectBound<2> a(dim);

    // We will set the low randomly and the width randomly for each dimension of
    // each bound.
    arma::vec lo_a(dim);
    arma::vec width_a(dim);

    lo_a.randu();
    width_a.randu();

    for (size_t j = 0; j < dim; j++)
      a[j] = Range(lo_a[j], lo_a[j] + width_a[j]);

    // Now run the test on a few points.
    for (int j = 0; j < 10; j++) {
      arma::vec point(dim);

      point.randu();

      Range r = a.RangeDistance(point);

      BOOST_REQUIRE_CLOSE(r.lo, a.MinDistance(point), 1e-5);
      BOOST_REQUIRE_CLOSE(r.hi, a.MaxDistance(point), 1e-5);
    }
  }
}

/***
 * Test that we can expand the bound to include a new point.
 */
BOOST_AUTO_TEST_CASE(HRectBoundOrOperatorPoint) {
  // Because this should be independent in each dimension, we can essentially
  // run five test cases at once.
  HRectBound<2> b(5);

  b[0] = Range(1.0, 3.0);
  b[1] = Range(2.0, 4.0);
  b[2] = Range(-2.0, -1.0);
  b[3] = Range(0.0, 0.0);
  b[4] = Range(); // Empty range.

  arma::vec point = "2.0 4.0 2.0 -1.0 6.0";

  b |= point;

  BOOST_REQUIRE_CLOSE(b[0].lo, 1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[0].hi, 3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[1].lo, 2.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[1].hi, 4.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[2].lo, -2.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[2].hi, 2.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[3].lo, -1.0, 1e-5);
  BOOST_REQUIRE_SMALL(b[3].hi, 1e-5);
  BOOST_REQUIRE_CLOSE(b[4].lo, 6.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[4].hi, 6.0, 1e-5);
}

/***
 * Test that we can expand the bound to include another bound.
 */
BOOST_AUTO_TEST_CASE(HRectBoundOrOperatorBound) {
  // Because this should be independent in each dimension, we can run many tests
  // at once.
  HRectBound<2> b(8);

  b[0] = Range(1.0, 3.0);
  b[1] = Range(2.0, 4.0);
  b[2] = Range(-2.0, -1.0);
  b[3] = Range(4.0, 5.0);
  b[4] = Range(2.0, 4.0);
  b[5] = Range(0.0, 0.0);
  b[6] = Range();
  b[7] = Range(1.0, 3.0);

  HRectBound<2> c(8);

  c[0] = Range(-3.0, -1.0); // Entirely less than the other bound.
  c[1] = Range(0.0, 2.0); // Touching edges.
  c[2] = Range(-3.0, -1.5); // Partially overlapping.
  c[3] = Range(4.0, 5.0); // Identical.
  c[4] = Range(1.0, 5.0); // Entirely enclosing.
  c[5] = Range(2.0, 2.0); // A single point.
  c[6] = Range(1.0, 3.0);
  c[7] = Range(); // Empty set.

  HRectBound<2> d = c;

  b |= c;
  d |= b;

  BOOST_REQUIRE_CLOSE(b[0].lo, -3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[0].hi, 3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[0].lo, -3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[0].hi, 3.0, 1e-5);

  BOOST_REQUIRE_CLOSE(b[1].lo, 0.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[1].hi, 4.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[1].lo, 0.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[1].hi, 4.0, 1e-5);

  BOOST_REQUIRE_CLOSE(b[2].lo, -3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[2].hi, -1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[2].lo, -3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[2].hi, -1.0, 1e-5);

  BOOST_REQUIRE_CLOSE(b[3].lo, 4.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[3].hi, 5.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[3].lo, 4.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[3].hi, 5.0, 1e-5);

  BOOST_REQUIRE_CLOSE(b[4].lo, 1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[4].hi, 5.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[4].lo, 1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[4].hi, 5.0, 1e-5);

  BOOST_REQUIRE_SMALL(b[5].lo, 1e-5);
  BOOST_REQUIRE_CLOSE(b[5].hi, 2.0, 1e-5);
  BOOST_REQUIRE_SMALL(d[5].lo, 1e-5);
  BOOST_REQUIRE_CLOSE(d[5].hi, 2.0, 1e-5);

  BOOST_REQUIRE_CLOSE(b[6].lo, 1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[6].hi, 3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[6].lo, 1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[6].hi, 3.0, 1e-5);

  BOOST_REQUIRE_CLOSE(b[7].lo, 1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(b[7].hi, 3.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[7].lo, 1.0, 1e-5);
  BOOST_REQUIRE_CLOSE(d[7].hi, 3.0, 1e-5);
}

/***
 * Test that the Contains() function correctly figures out whether or not a
 * point is in a bound.
 */
BOOST_AUTO_TEST_CASE(HRectBoundContains) {
  // We can test a couple different points: completely outside the bound,
  // adjacent in one dimension to the bound, adjacent in all dimensions to the
  // bound, and inside the bound.
  HRectBound<2> b(3);

  b[0] = Range(0.0, 2.0);
  b[1] = Range(0.0, 2.0);
  b[2] = Range(0.0, 2.0);

  // Completely outside the range.
  arma::vec point = "-1.0 4.0 4.0";
  BOOST_REQUIRE(!b.Contains(point));

  // Completely outside, but one dimension is in the range.
  point = "-1.0 4.0 1.0";
  BOOST_REQUIRE(!b.Contains(point));

  // Outside, but one dimension is on the edge.
  point = "-1.0 0.0 3.0";
  BOOST_REQUIRE(!b.Contains(point));

  // Two dimensions are on the edge, but one is outside.
  point = "0.0 0.0 3.0";
  BOOST_REQUIRE(!b.Contains(point));

  // Completely on the edge (should be contained).
  point = "0.0 0.0 0.0";
  BOOST_REQUIRE(b.Contains(point));

  // Inside the range.
  point = "0.3 1.0 0.4";
  BOOST_REQUIRE(b.Contains(point));
}

BOOST_AUTO_TEST_CASE(TestBallBound) {
  DBallBound<> b1;
  DBallBound<> b2;

  // Create two balls with a center distance of 1 from each other.
  // Give the first one a radius of 0.3 and the second a radius of 0.4.

  b1.center().set_size(3);
  b1.center()[0] = 1;
  b1.center()[1] = 2;
  b1.center()[2] = 3;
  b1.set_radius(0.3);

  b2.center().set_size(3);
  b2.center()[0] = 1;
  b2.center()[1] = 2;
  b2.center()[2] = 4;
  b2.set_radius(0.4);

  BOOST_REQUIRE_CLOSE(sqrt(b1.MinDistanceSq(b2)), 1-0.3-0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b1.RangeDistanceSq(b2).hi), 1+0.3+0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b1.RangeDistanceSq(b2).lo), 1-0.3-0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(b1.RangeDistance(b2).hi, 1+0.3+0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(b1.RangeDistance(b2).lo, 1-0.3-0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b1.MinToMidSq(b2)), 1-0.3, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b1.MinimaxDistanceSq(b2)), 1-0.3+0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b1.MidDistanceSq(b2)), 1.0, 1e-5);

  BOOST_REQUIRE_CLOSE(sqrt(b2.MinDistanceSq(b1)), 1-0.3-0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.MaxDistanceSq(b1)), 1+0.3+0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.RangeDistanceSq(b1).hi), 1+0.3+0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.RangeDistanceSq(b1).lo), 1-0.3-0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.MinToMidSq(b1)), 1-0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.MinimaxDistanceSq(b1)), 1-0.4+0.3, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.MidDistanceSq(b1)), 1.0, 1e-5);

  BOOST_REQUIRE(b1.Contains(b1.center()));
  BOOST_REQUIRE(!b1.Contains(b2.center()));

  BOOST_REQUIRE(!b2.Contains(b1.center()));
  BOOST_REQUIRE(b2.Contains(b2.center()));
  arma::vec b2point(3); // a point that's within the radius bot not the center
  b2point[0] = 1.1;
  b2point[1] = 2.1;
  b2point[2] = 4.1;

  BOOST_REQUIRE(b2.Contains(b2point));

  BOOST_REQUIRE_SMALL(sqrt(b1.MinDistanceSq(b1.center())), 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b1.MinDistanceSq(b2.center())), 1 - 0.3, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.MinDistanceSq(b1.center())), 1 - 0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b2.MaxDistanceSq(b1.center())), 1 + 0.4, 1e-5);
  BOOST_REQUIRE_CLOSE(sqrt(b1.MaxDistanceSq(b2.center())), 1 + 0.3, 1e-5);
}

/***
 * It seems as though Bill has stumbled across a bug where
 * BinarySpaceTree<>::count() returns something different than
 * BinarySpaceTree<>::count_.  So, let's build a simple tree and make sure they
 * are the same.
 */
BOOST_AUTO_TEST_CASE(tree_count_mismatch) {
  arma::mat dataset = "2.0 5.0 9.0 4.0 8.0 7.0;"
                      "3.0 4.0 6.0 7.0 1.0 2.0 ";

  // Leaf size of 1.
  IO::GetParam<int>("tree/leaf_size") = 1;
  BinarySpaceTree<HRectBound<2> > root_node(dataset);

  BOOST_REQUIRE(root_node.count() == 6);
  BOOST_REQUIRE(root_node.left()->count() == 3);
  BOOST_REQUIRE(root_node.left()->left()->count() == 2);
  BOOST_REQUIRE(root_node.left()->left()->left()->count() == 1);
  BOOST_REQUIRE(root_node.left()->left()->right()->count() == 1);
  BOOST_REQUIRE(root_node.left()->right()->count() == 1);
  BOOST_REQUIRE(root_node.right()->count() == 3);
  BOOST_REQUIRE(root_node.right()->left()->count() == 2);
  BOOST_REQUIRE(root_node.right()->left()->left()->count() == 1);
  BOOST_REQUIRE(root_node.right()->left()->right()->count() == 1);
  BOOST_REQUIRE(root_node.right()->right()->count() == 1);
}


BOOST_AUTO_TEST_CASE(kd_tree_test) {
  size_t max_points = 10000;
  size_t dimensions = 3;
  // Generate the dataset.
  srand(time(NULL));
  size_t size = rand() % max_points;
  arma::mat dataset = arma::mat(dimensions, size);
  arma::mat datacopy;

  // Mappings for post-sort verification of data.
  std::vector<size_t> new_to_old;
  std::vector<size_t> old_to_new;

  // Generate data.
  dataset.randu(dimensions, size);
  datacopy = dataset;

  // Check validity of tree data
  BinarySpaceTree<HRectBound<2> > root(dataset, new_to_old, old_to_new);

  BOOST_REQUIRE_EQUAL(root.count(), size);

  for(size_t i = 0; i < size; i++) {
    for(size_t j = 0; j < dimensions; j++) {
      // Check mappings.
      BOOST_REQUIRE_EQUAL(dataset(j, i), datacopy(j, new_to_old[i]));
      BOOST_REQUIRE_EQUAL(dataset(j, old_to_new[i]), datacopy(j, i));
    }
  }
}
