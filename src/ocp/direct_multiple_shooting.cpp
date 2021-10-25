#include "robotoc/ocp/direct_multiple_shooting.hpp"

#include <omp.h>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace robotoc{

DirectMultipleShooting::DirectMultipleShooting(const int N, 
                                               const int max_num_impulse, 
                                               const int nthreads) 
  : max_num_impulse_(max_num_impulse),
    nthreads_(nthreads),
    kkt_error_(Eigen::VectorXd::Zero(N+1+4*max_num_impulse)) {
  try {
    if (max_num_impulse < 0) {
      throw std::out_of_range(
          "invalid value: max_num_impulse must be non-negative!");
    }
    if (nthreads <= 0) {
      throw std::out_of_range("invalid value: nthreads must be positive!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
}


DirectMultipleShooting::DirectMultipleShooting()
  : max_num_impulse_(0),
    nthreads_(0),
    kkt_error_() {
}


DirectMultipleShooting::~DirectMultipleShooting() {
}


void DirectMultipleShooting::initConstraints(
    OCP& ocp, aligned_vector<Robot>& robots, 
    const std::shared_ptr<ContactSequence>& contact_sequence, 
    const Solution& s) const {
  const int N = ocp.discrete().N();
  const int N_impulse = ocp.discrete().N_impulse();
  const int N_lift = ocp.discrete().N_lift();
  const int N_all = N + 1 + 2*N_impulse + N_lift;
  #pragma omp parallel for num_threads(nthreads_)
  for (int i=0; i<N_all; ++i) {
    if (i < N) {
      ocp[i].initConstraints(robots[omp_get_thread_num()], i, s[i]);
    }
    else if (i == N) {
      ocp.terminal.initConstraints(robots[omp_get_thread_num()], N, s[N]);
    }
    else if (i < N+1+N_impulse) {
      const int impulse_index  = i - (N+1);
      ocp.impulse[impulse_index].initConstraints(robots[omp_get_thread_num()], 
                                                 s.impulse[impulse_index]);
    }
    else if (i < N+1+2*N_impulse) {
      const int impulse_index  = i - (N+1+N_impulse);
      ocp.aux[impulse_index].initConstraints(robots[omp_get_thread_num()], 0, 
                                             s.aux[impulse_index]);
    }
    else {
      const int lift_index = i - (N+1+2*N_impulse);
      ocp.lift[lift_index].initConstraints(robots[omp_get_thread_num()], 0, 
                                           s.lift[lift_index]);
    }
  }
}


void DirectMultipleShooting::computeKKTResidual(
    OCP& ocp, aligned_vector<Robot>& robots, 
    const std::shared_ptr<ContactSequence>& contact_sequence, 
    const Eigen::VectorXd& q, const Eigen::VectorXd& v, const Solution& s, 
    KKTMatrix& kkt_matrix, KKTResidual& kkt_residual) const {
  runParallel<internal::ComputeKKTResidual>(ocp, robots, contact_sequence, q, v,  
                                            s, kkt_matrix, kkt_residual);
}


void DirectMultipleShooting::computeKKTSystem(
    OCP& ocp, aligned_vector<Robot>& robots, 
    const std::shared_ptr<ContactSequence>& contact_sequence, 
    const Eigen::VectorXd& q, const Eigen::VectorXd& v, const Solution& s, 
    KKTMatrix& kkt_matrix, KKTResidual& kkt_residual) const {
  runParallel<internal::ComputeKKTSystem>(ocp, robots, contact_sequence, q, v, 
                                          s, kkt_matrix, kkt_residual);
}


double DirectMultipleShooting::KKTError(const OCP& ocp, 
                                        const KKTResidual& kkt_residual) {
  const int N = ocp.discrete().N();
  const int N_impulse = ocp.discrete().N_impulse();
  const int N_lift = ocp.discrete().N_lift();
  const int N_all = N + 1 + 3*N_impulse + N_lift;
  #pragma omp parallel for num_threads(nthreads_)
  for (int i=0; i<N_all; ++i) {
    if (i < N) {
      kkt_error_.coeffRef(i) 
          = ocp[i].KKTError(kkt_residual[i], ocp.discrete().dt(i));
    }
    else if (i == N) {
      kkt_error_.coeffRef(N) 
          = ocp.terminal.KKTError(kkt_residual[N]);
    }
    else if (i < N+1+N_impulse) {
      const int impulse_index = i - (N+1);
      const int time_stage_before_impulse 
          = ocp.discrete().timeStageBeforeImpulse(impulse_index);
      kkt_error_.coeffRef(i) 
          = ocp.impulse[impulse_index].KKTError(kkt_residual.impulse[impulse_index]);
    }
    else if (i < N+1+2*N_impulse) {
      const int impulse_index = i - (N+1+N_impulse);
      kkt_error_.coeffRef(i) 
          = ocp.aux[impulse_index].KKTError(kkt_residual.aux[impulse_index], 
                                            ocp.discrete().dt_aux(impulse_index));
      const int time_stage_before_impulse 
          = ocp.discrete().timeStageBeforeImpulse(impulse_index);
      if (ocp.discrete().isSTOEnabledImpulse(impulse_index)) {
        if (time_stage_before_impulse >= 1) {
          const double hdiff = kkt_residual[time_stage_before_impulse-1].h 
                                + kkt_residual[time_stage_before_impulse].h 
                                - kkt_residual.aux[impulse_index].h;
          kkt_error_.coeffRef(i) += hdiff*hdiff;
        }
        else {
          const double hdiff = kkt_residual[time_stage_before_impulse].h 
                                - kkt_residual.aux[impulse_index].h;
          kkt_error_.coeffRef(i) += hdiff*hdiff;
        }
      }
    }
    else if (i < N+1+2*N_impulse+N_lift) {
      const int lift_index = i - (N+1+2*N_impulse);
      kkt_error_.coeffRef(i) 
          = ocp.lift[lift_index].KKTError(kkt_residual.lift[lift_index], 
                                          ocp.discrete().dt_lift(lift_index));
      const int time_stage_before_lift
          = ocp.discrete().timeStageBeforeLift(lift_index);
      if (ocp.discrete().isSTOEnabledLift(lift_index)) {
        const double hdiff = kkt_residual[time_stage_before_lift].h 
                              - kkt_residual.lift[lift_index].h;
        kkt_error_.coeffRef(i) += hdiff*hdiff;
      }
    }
    else {
      const int impulse_index = i - (N+1+2*N_impulse+N_lift);
      kkt_error_.coeffRef(i) = kkt_residual.switching[impulse_index].KKTError();
    }
  }
  return std::sqrt(kkt_error_.head(N_all).sum());
}


double DirectMultipleShooting::totalCost(const OCP& ocp) const {
  double total_cost = 0;
  for (int i=0; i<ocp.discrete().N(); ++i) {
    total_cost += ocp[i].stageCost();
  }
  total_cost += ocp.terminal.terminalCost();
  for (int i=0; i<ocp.discrete().N_impulse(); ++i) {
    total_cost += ocp.impulse[i].stageCost();
    total_cost += ocp.aux[i].stageCost();
  }
  for (int i=0; i<ocp.discrete().N_lift(); ++i) {
    total_cost += ocp.lift[i].stageCost();
  }
  return total_cost;
}


void DirectMultipleShooting::computeInitialStateDirection(
    const OCP& ocp, const aligned_vector<Robot>& robots, 
    const Eigen::VectorXd& q0, const Eigen::VectorXd& v0, 
    const Solution& s, Direction& d) {
  ocp[0].computeInitialStateDirection(robots[0], q0, v0, s[0], d[0]);
}


void DirectMultipleShooting::integrateSolution(
    OCP& ocp, const aligned_vector<Robot>& robots, 
    const double primal_step_size, const double dual_step_size, 
    Direction& d, Solution& s) const {
  assert(robots.size() == nthreads_);
  const int N = ocp.discrete().N();
  const int N_impulse = ocp.discrete().N_impulse();
  const int N_lift = ocp.discrete().N_lift();
  const int N_all = N + 1 + 2*N_impulse + N_lift;
  #pragma omp parallel for num_threads(nthreads_)
  for (int i=0; i<N_all; ++i) {
    if (i < N) {
      if (ocp.discrete().isTimeStageBeforeImpulse(i)) {
        const int impulse_index = ocp.discrete().impulseIndexAfterTimeStage(i);
        const bool sto = ocp.discrete().isSTOEnabledImpulse(impulse_index);
        ocp[i].expandDual(ocp.discrete().dt(i), d.impulse[impulse_index], d[i], sto);
      }
      else if (ocp.discrete().isTimeStageBeforeLift(i)) {
        const int lift_index = ocp.discrete().liftIndexAfterTimeStage(i);
        const bool sto = ocp.discrete().isSTOEnabledLift(lift_index);
        ocp[i].expandDual(ocp.discrete().dt(i), d.lift[lift_index], d[i], sto);
      }
      else {
        constexpr bool sto = false;
        ocp[i].expandDual(ocp.discrete().dt(i), d[i+1], d[i], sto);
      }
      ocp[i].updatePrimal(robots[omp_get_thread_num()], primal_step_size, 
                          d[i], s[i]);
      ocp[i].updateDual(dual_step_size);
    }
    else if (i == N) {
      ocp.terminal.expandDual(d[N]);
      ocp.terminal.updatePrimal(robots[omp_get_thread_num()], primal_step_size, 
                                d[N], s[N]);
      ocp.terminal.updateDual(dual_step_size);
    }
    else if (i < N+1+N_impulse) {
      const int impulse_index  = i - (N+1);
      ocp.impulse[impulse_index].expandDual(d.aux[impulse_index], 
                                            d.impulse[impulse_index]);
      ocp.impulse[impulse_index].updatePrimal(robots[omp_get_thread_num()], 
                                              primal_step_size, 
                                              d.impulse[impulse_index], 
                                              s.impulse[impulse_index]);
      ocp.impulse[impulse_index].updateDual(dual_step_size);
    }
    else if (i < N+1+2*N_impulse) {
      const int impulse_index  = i - (N+1+N_impulse);
      const bool sto = ocp.discrete().isSTOEnabledImpulse(impulse_index);
      ocp.aux[impulse_index].expandDual(
          ocp.discrete().dt_aux(impulse_index), 
          d[ocp.discrete().timeStageAfterImpulse(impulse_index)], 
          d.aux[impulse_index], sto);
      ocp.aux[impulse_index].updatePrimal(robots[omp_get_thread_num()], 
                                          primal_step_size, 
                                          d.aux[impulse_index], 
                                          s.aux[impulse_index]);
      ocp.aux[impulse_index].updateDual(dual_step_size);
    }
    else {
      const int lift_index = i - (N+1+2*N_impulse);
      const bool sto = ocp.discrete().isSTOEnabledLift(lift_index);
      ocp.lift[lift_index].expandDual(
          ocp.discrete().dt_lift(lift_index), 
          d[ocp.discrete().timeStageAfterLift(lift_index)], 
          d.lift[lift_index], sto);
      ocp.lift[lift_index].updatePrimal(robots[omp_get_thread_num()], 
                                        primal_step_size, d.lift[lift_index], 
                                        s.lift[lift_index]);
      ocp.lift[lift_index].updateDual(dual_step_size);
    }
  }
}

} // namespace robotoc