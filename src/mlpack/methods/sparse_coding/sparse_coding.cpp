/**
 * @file sparse_coding.cpp
 * @author Nishant Mehta
 *
 * Implementation of Sparse Coding with Dictionary Learning using l1 (LASSO) or
 * l1+l2 (Elastic Net) regularization.
 */
#include "sparse_coding.hpp"

using namespace std;
using namespace arma;
using namespace mlpack;
using namespace mlpack::regression;
using namespace mlpack::sparse_coding;

// TODO: parameterizable; options to methods?
#define OBJ_TOL 1e-2 // 1E-9
#define NEWTON_TOL 1e-6 // 1E-9

SparseCoding::SparseCoding(const mat& data,
                           const size_t atoms,
                           const double lambda1,
                           const double lambda2) :
    atoms(atoms),
    data(data),
    codes(mat(atoms, data.n_cols)),
    lambda1(lambda1),
    lambda2(lambda2)
{ /* Nothing left to do. */ }

// Always a not good decision!
void SparseCoding::RandomInitDictionary() {
  dictionary = randn(data.n_rows, atoms);

  for (size_t j = 0; j < atoms; ++j)
    dictionary.col(j) /= norm(dictionary.col(j), 2);
}

// The sensible heuristic.
void SparseCoding::DataDependentRandomInitDictionary()
{
  dictionary = mat(data.n_rows, atoms);
  for (size_t j = 0; j < atoms; ++j)
  {
    vec vecD_j = dictionary.unsafe_col(j);
    RandomAtom(vecD_j);
  }
}

void SparseCoding::RandomAtom(vec& atom)
{
  atom.zeros();
  const size_t nSeedAtoms = 3;
  for (size_t i = 0; i < nSeedAtoms; i++)
    atom += data.col(rand() % data.n_cols);

  atom /= norm(atom, 2);
}

void SparseCoding::DoSparseCoding(const size_t maxIterations)
{
  double lastObjVal = DBL_MAX;

  Log::Info << "Initial Coding Step." << endl;

  OptimizeCode();
  uvec adjacencies = find(codes);

  Log::Info << "  Sparsity level: "
      << 100.0 * ((double) (adjacencies.n_elem)) / ((double)
      (atoms * data.n_cols)) << "%" << endl;
  Log::Info << "  Objective value: " << Objective() << "." << endl;

  for (size_t t = 1; t != maxIterations; ++t)
  {
    Log::Info << "Iteration " << t << " of " << maxIterations << "." << endl;

    Log::Info << "Performing dictionary step... ";
    OptimizeDictionary(adjacencies);
    Log::Info << "objective value: " << Objective() << "." << endl;

    Log::Info << "Performing coding step..." << endl;
    OptimizeCode();
    adjacencies = find(codes);
    Log::Info << "  Sparsity level: "
        << 100.0 *
        ((double) (adjacencies.n_elem)) / ((double) (atoms * data.n_cols))
        << "%" << endl;

    double curObjVal = Objective();
    Log::Info << "  Objective value: " << curObjVal << "." << endl;

    double objValImprov = lastObjVal - curObjVal;
    Log::Info << "  Improvement: " << scientific << objValImprov << "." << endl;

    if (objValImprov < OBJ_TOL)
    {
      Log::Info << "Converged within tolerance " << OBJ_TOL << ".\n";
      break;
    }

    lastObjVal = curObjVal;
  }
}

void SparseCoding::OptimizeCode()
{
  // When using Cholesky version of LARS, this is correct even if lambda2 > 0.
  mat matGram = trans(dictionary) * dictionary;
  // mat matGram;
  // if(lambda2 > 0) {
  //   matGram = trans(dictionary) * dictionary + lambda2 * eye(atoms, atoms);
  // }
  // else {
  //   matGram = trans(dictionary) * dictionary;
  // }

  for (size_t i = 0; i < data.n_cols; ++i)
  {
    // Report progress.
    if ((i % 100) == 0)
      Log::Debug << "Optimization at point " << i << "." << endl;

    bool useCholesky = true;
    LARS lars(useCholesky, matGram, lambda1, lambda2);

    //lars->SetGramMem(matGram.memptr(), matGram.n_rows);
    lars.DoLARS(dictionary, data.unsafe_col(i));

    vec beta;
    lars.Solution(beta);
    codes.col(i) = beta;
  }
}

void SparseCoding::OptimizeDictionary(const uvec& adjacencies)
{
  // Count the number of atomic neighbors for each point x^i.
  uvec neighborCounts = zeros<uvec>(data.n_cols, 1);

  if (adjacencies.n_elem > 0)
  {
    // This gets the column index.
    // TODO: is this integer division intentional?
    size_t curPointInd = (size_t) (adjacencies(0) / atoms);
    size_t curCount = 1;

    for (size_t l = 1; l < adjacencies.n_elem; ++l)
    {
      if ((size_t) (adjacencies(l) / atoms) == curPointInd)
      {
        ++curCount;
      }
      else
      {
        neighborCounts(curPointInd) = curCount;
        curPointInd = (size_t) (adjacencies(l) / atoms);
        curCount = 1;
      }
    }

    neighborCounts(curPointInd) = curCount;
  }

  // Handle the case of inactive atoms (atoms not used in the given coding).
  std::vector<size_t> inactiveAtoms;
  std::vector<size_t> activeAtoms;
  activeAtoms.reserve(atoms);

  for (size_t j = 0; j < atoms; ++j)
  {
    if (accu(codes.row(j) != 0) == 0)
      inactiveAtoms.push_back(j);
    else
      activeAtoms.push_back(j);
  }

  const size_t nActiveAtoms = activeAtoms.size();
  const size_t nInactiveAtoms = inactiveAtoms.size();

  // Efficient construction of Z restricted to active atoms.
  mat matActiveZ;
  if (inactiveAtoms.empty())
  {
    matActiveZ = codes;
  }
  else
  {
    uvec inactiveAtomsVec = conv_to<uvec>::from(inactiveAtoms);
    RemoveRows(codes, inactiveAtomsVec, matActiveZ);
  }

  uvec atomReverseLookup = uvec(atoms);
  for (size_t i = 0; i < nActiveAtoms; ++i)
    atomReverseLookup(activeAtoms[i]) = i;

  if (nInactiveAtoms > 0)
  {
    Log::Info << "There are " << nInactiveAtoms
        << " inactive atoms. They will be re-initialized randomly.\n";
  }

  Log::Debug << "Solving Dual via Newton's Method.\n";

  mat dictionaryEstimate;
  // Solve using Newton's method in the dual - note that the final dot
  // multiplication with inv(A) seems to be unavoidable. Although more
  // expensive, the code written this way (we use solve()) should be more
  // numerically stable than just using inv(A) for everything.
  vec dualVars = zeros<vec>(nActiveAtoms);

  //vec dualVars = 1e-14 * ones<vec>(nActiveAtoms);

  // Method used by feature sign code - fails miserably here.  Perhaps the
  // MATLAB optimizer fmincon does something clever?
  //vec dualVars = 10.0 * randu(nActiveAtoms, 1);

  //vec dualVars = diagvec(solve(dictionary, data * trans(codes))
  //    - codes * trans(codes));
  //for (size_t i = 0; i < dualVars.n_elem; i++)
  //  if (dualVars(i) < 0)
  //    dualVars(i) = 0;

  bool converged = false;
  mat codesXT = matActiveZ * trans(data);
  mat codesZT = matActiveZ * trans(matActiveZ);

  for (size_t t = 1; !converged; ++t)
  {
    mat A = codesZT + diagmat(dualVars);

    mat matAInvZXT = solve(A, codesXT);

    vec gradient = -(sum(square(matAInvZXT), 1) - ones<vec>(nActiveAtoms));

    mat hessian = -(-2 * (matAInvZXT * trans(matAInvZXT)) % inv(A));

    vec searchDirection = -solve(hessian, gradient);
    //vec searchDirection = -gradient;

    // Armijo line search.
    const double c = 1e-4;
    double alpha = 1.0;
    const double rho = 0.9;
    double sufficientDecrease = c * dot(gradient, searchDirection);

    /*
    {
      double sumDualVars = sum(dualVars);
      double fOld = -(-trace(trans(codesXT) * matAInvZXT) - sumDualVars);
      Log::Debug << "fOld = " << fOld << "." << endl;
      double fNew =
          -(-trace(trans(codesXT) * solve(codesZT +
          diagmat(dualVars + alpha * searchDirection), codesXT))
          - (sumDualVars + alpha * sum(searchDirection)) );
      Log::Debug << "fNew = " << fNew << "." << endl;
    }
    */

    double improvement;
    while (true)
    {
      // Calculate objective.
      double sumDualVars = sum(dualVars);
      double fOld = -(-trace(trans(codesXT) * matAInvZXT) - sumDualVars);
      double fNew = -(-trace(trans(codesXT) * solve(codesZT +
          diagmat(dualVars + alpha * searchDirection), codesXT)) -
          (sumDualVars + alpha * sum(searchDirection)));

      if (fNew <= fOld + alpha * sufficientDecrease)
      {
        searchDirection = alpha * searchDirection;
        improvement = fOld - fNew;
        break;
      }

      alpha *= rho;
    }

    // End of Armijo line search code.

    dualVars += searchDirection;
    double normGradient = norm(gradient, 2);
    Log::Debug << "Newton Method iteration " << t << ":" << endl;
    Log::Debug << "  Gradient norm: " << std::scientific << normGradient
        << "." << endl;
    Log::Debug << "  Improvement: " << std::scientific << improvement << ".\n";

    if (improvement < NEWTON_TOL)
      converged = true;
  }

  if (inactiveAtoms.empty())
  {
    dictionaryEstimate = trans(solve(codesZT + diagmat(dualVars), codesXT));
  }
  else
  {
    mat dictionaryActiveEstimate = trans(solve(codesZT + diagmat(dualVars),
        codesXT));
    dictionaryEstimate = zeros(data.n_rows, atoms);

    for (size_t i = 0; i < nActiveAtoms; ++i)
      dictionaryEstimate.col(activeAtoms[i]) = dictionaryActiveEstimate.col(i);

    for (size_t i = 0; i < nInactiveAtoms; ++i)
    {
      vec vecdictionaryi = dictionaryEstimate.unsafe_col(inactiveAtoms[i]);
      RandomAtom(vecdictionaryi);
    }
  }

  dictionary = dictionaryEstimate;
}

void SparseCoding::ProjectDictionary()
{
  for (size_t j = 0; j < atoms; j++)
  {
    double normD_j = norm(dictionary.col(j), 2);
    if ((normD_j > 1) && (normD_j - 1.0 > 1e-9))
    {
      Log::Warn << "Norm exceeded 1 by " << std::scientific << normD_j - 1.0
          << ".  Shrinking...\n";
      dictionary.col(j) /= normD_j;
    }
  }
}

double SparseCoding::Objective()
{
  double l11NormZ = sum(sum(abs(codes)));
  double froNormResidual = norm(data - dictionary * codes, "fro");

  if (lambda2 > 0)
  {
    double froNormZ = norm(codes, "fro");
    return 0.5 *
      (froNormResidual * froNormResidual + lambda2 * froNormZ * froNormZ) +
      lambda1 * l11NormZ;
  }
  else
  {
    return 0.5 * froNormResidual * froNormResidual + lambda1 * l11NormZ;
  }
}

void mlpack::sparse_coding::RemoveRows(const mat& X,
                                       uvec rows_to_remove,
                                       mat& X_mod)
{
  const size_t n_cols = X.n_cols;
  const size_t n_rows = X.n_rows;
  const size_t n_to_remove = rows_to_remove.n_elem;
  const size_t n_to_keep = n_rows - n_to_remove;

  if (n_to_remove == 0)
  {
    X_mod = X;
  }
  else
  {
    X_mod.set_size(n_to_keep, n_cols);

    size_t cur_row = 0;
    size_t remove_ind = 0;
    // First, check 0 to first row to remove.
    if (rows_to_remove(0) > 0)
    {
      // Note that this implies that n_rows > 1.
      size_t height = rows_to_remove(0);
      X_mod(span(cur_row, cur_row + height - 1), span::all) =
          X(span(0, rows_to_remove(0) - 1), span::all);
      cur_row += height;
    }
    // Now, check i'th row to remove to (i + 1)'th row to remove, until i is the
    // penultimate row.
    while (remove_ind < n_to_remove - 1)
    {
      size_t height = rows_to_remove[remove_ind + 1] -
          rows_to_remove[remove_ind] - 1;

      if (height > 0)
      {
        X_mod(span(cur_row, cur_row + height - 1), span::all) =
            X(span(rows_to_remove[remove_ind] + 1,
            rows_to_remove[remove_ind + 1] - 1), span::all);
        cur_row += height;
      }

      remove_ind++;
    }

    // Now that i is the last row to remove, check last row to remove to last
    // row.
    if (rows_to_remove[remove_ind] < n_rows - 1)
    {
      X_mod(span(cur_row, n_to_keep - 1), span::all) =
          X(span(rows_to_remove[remove_ind] + 1, n_rows - 1), span::all);
    }
  }
}
