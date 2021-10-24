#include "robotoc/cost/local_contact_force_cost.hpp"

#include <iostream>
#include <stdexcept>


namespace robotoc {

LocalContactForceCost::LocalContactForceCost(const Robot& robot)
  : CostFunctionComponentBase(),
    max_point_contacts_(robot.maxPointContacts()),
    max_dimf_(robot.max_dimf()),
    f_ref_(robot.maxPointContacts(), Eigen::Vector3d::Zero()),
    f_weight_(robot.maxPointContacts(), Eigen::Vector3d::Zero()),
    fi_ref_(robot.maxPointContacts(), Eigen::Vector3d::Zero()),
    fi_weight_(robot.maxPointContacts(), Eigen::Vector3d::Zero()) {
}


LocalContactForceCost::LocalContactForceCost()
  : CostFunctionComponentBase(),
    max_point_contacts_(0),
    max_dimf_(0),
    f_ref_(),
    f_weight_(),
    fi_ref_(),
    fi_weight_() {
}


LocalContactForceCost::~LocalContactForceCost() {
}


void LocalContactForceCost::set_f_ref(
    const std::vector<Eigen::Vector3d>& f_ref) {
  try {
    if (f_ref.size() != max_point_contacts_) {
      throw std::invalid_argument(
          "invalid size: f_ref.size() must be " 
          + std::to_string(max_point_contacts_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  f_ref_ = f_ref;
}


void LocalContactForceCost::set_f_weight(
    const std::vector<Eigen::Vector3d>& f_weight) {
  try {
    if (f_weight.size() != max_point_contacts_) {
      throw std::invalid_argument(
          "invalid size: f_weight.size() must be " 
          + std::to_string(max_point_contacts_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  f_weight_ = f_weight;
}


void LocalContactForceCost::set_fi_ref(
    const std::vector<Eigen::Vector3d>& fi_ref) {
  try {
    if (fi_ref.size() != max_point_contacts_) {
      throw std::invalid_argument(
          "invalid size: f_ref.size() must be " 
          + std::to_string(max_point_contacts_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  fi_ref_ = fi_ref;
}


void LocalContactForceCost::set_fi_weight(
    const std::vector<Eigen::Vector3d>& fi_weight) {
  try {
    if (fi_weight.size() != max_point_contacts_) {
      throw std::invalid_argument(
          "invalid size: f_weight.size() must be " 
          + std::to_string(max_point_contacts_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  fi_weight_ = fi_weight;
}


bool LocalContactForceCost::useKinematics() const {
  return false;
}


double LocalContactForceCost::evalStageCost(Robot& robot, CostFunctionData& data, 
                                            const double t, const double dt, 
                                            const SplitSolution& s) const {
  double l = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isContactActive(i)) {
      l += (f_weight_[i].array() * (s.f[i].array()-f_ref_[i].array()) 
                                 * (s.f[i].array()-f_ref_[i].array())).sum();
    }
  }
  return 0.5 * dt * l;
}


void LocalContactForceCost::evalStageCostDerivatives(
    Robot& robot, CostFunctionData& data, const double t, const double dt, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  int dimf_stack = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isContactActive(i)) {
      kkt_residual.lf().template segment<3>(dimf_stack).array()
          += dt * f_weight_[i].array() * (s.f[i].array()-f_ref_[i].array());
      dimf_stack += 3;
    }
  }
}


void LocalContactForceCost::evalStageCostHessian(
    Robot& robot, CostFunctionData& data, const double t, const double dt, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix) const {
  int dimf_stack = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isContactActive(i)) {
      kkt_matrix.Qff().diagonal().template segment<3>(dimf_stack).noalias() 
          += dt * f_weight_[i];
      dimf_stack += 3;
    }
  }
}


double LocalContactForceCost::evalTerminalCost(Robot& robot, 
                                               CostFunctionData& data, 
                                               const double t, 
                                               const SplitSolution& s) const {
  return 0;
}


void LocalContactForceCost::evalTerminalCostDerivatives(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  // Do nothing.
}


void LocalContactForceCost::evalTerminalCostHessian(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix) const {
  // Do nothing.
}


double LocalContactForceCost::evalImpulseCost(
    Robot& robot, CostFunctionData& data, const double t, 
    const ImpulseSplitSolution& s) const {
  double l = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isImpulseActive(i)) {
      l += (fi_weight_[i].array() * (s.f[i].array()-fi_ref_[i].array()) 
                                  * (s.f[i].array()-fi_ref_[i].array())).sum();
    }
  }
  return 0.5 * l;
}


void LocalContactForceCost::evalImpulseCostDerivatives(
    Robot& robot, CostFunctionData& data, const double t, 
    const ImpulseSplitSolution& s, 
    ImpulseSplitKKTResidual& kkt_residual) const {
  int dimf_stack = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isImpulseActive(i)) {
      kkt_residual.lf().template segment<3>(dimf_stack).array()
          += fi_weight_[i].array() * (s.f[i].array()-fi_ref_[i].array());
      dimf_stack += 3;
    }
  }
}


void LocalContactForceCost::evalImpulseCostHessian(
    Robot& robot, CostFunctionData& data, const double t, 
    const ImpulseSplitSolution& s, ImpulseSplitKKTMatrix& kkt_matrix) const {
  int dimf_stack = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isImpulseActive(i)) {
      kkt_matrix.Qff().diagonal().template segment<3>(dimf_stack).noalias() 
          += fi_weight_[i];
      dimf_stack += 3;
    }
  }
}

} // namespace robotoc