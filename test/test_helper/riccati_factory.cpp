#include "riccati_factory.hpp"

#include "robotoc/hybrid/hybrid_ocp_discretization.hpp"


namespace robotoc {
namespace testhelper {

SplitRiccatiFactorization CreateSplitRiccatiFactorization(const Robot& robot) {
  SplitRiccatiFactorization riccati_factorization(robot);
  const int dimx = 2*robot.dimv();
  Eigen::MatrixXd seed = Eigen::MatrixXd::Random(dimx, dimx);
  riccati_factorization.P = seed * seed.transpose();
  riccati_factorization.s.setRandom();
  return riccati_factorization;
}

} // namespace testhelper
} // namespace robotoc