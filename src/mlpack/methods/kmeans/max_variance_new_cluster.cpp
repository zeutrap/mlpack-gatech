/**
 * @file max_variance_new_cluster.cpp
 * @author Ryan Curtin
 *
 * Implementation of MaxVarianceNewCluster class.
 */
#include "max_variance_new_cluster.hpp"

using namespace mlpack;
using namespace kmeans;

/**
 * Take action about an empty cluster.
 */
size_t MaxVarianceNewCluster::EmptyCluster(const arma::mat& data,
                                           const size_t emptyCluster,
                                           const arma::mat& centroids,
                                           arma::Col<size_t>& clusterCounts,
                                           arma::Col<size_t>& assignments)
{
  // First, we need to find the cluster with maximum variance (by which I mean
  // the sum of the covariance matrices).
  arma::vec variances;
  variances.zeros(clusterCounts.n_elem); // Start with 0.

  // Add the variance of each point's distance away from the cluster.  I think
  // this is the sensible thing to do.
  for (size_t i = 0; i < data.n_cols; i++)
  {
    arma::vec diff = data.col(i) - centroids.col(assignments[i]);
    variances[assignments[i]] += var(diff);
  }

  // Now find the cluster with maximum variance.
  arma::u32 maxVarCluster;
  variances.max(maxVarCluster);

  // Now, inside this cluster, find the point which is furthest away.
  size_t furthestPoint = data.n_cols;
  double maxDistance = 0;
  for (size_t i = 0; i < data.n_cols; i++)
  {
    if (assignments[i] == maxVarCluster)
    {
      arma::vec diff = data.col(i) - centroids.col(maxVarCluster);
      double distance = var(diff);

      if (distance > maxDistance)
      {
        maxDistance = distance;
        furthestPoint = i;
      }
    }
  }

  // Take that point and add it to the empty cluster.
  clusterCounts[maxVarCluster]--;
  clusterCounts[emptyCluster]++;
  assignments[furthestPoint] = emptyCluster;

  return 1; // We only changed one point.
}
