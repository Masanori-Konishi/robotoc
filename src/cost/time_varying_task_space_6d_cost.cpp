#include "robotoc/cost/time_varying_task_space_6d_cost.hpp"


namespace robotoc {

TimeVaryingTaskSpace6DCost::TimeVaryingTaskSpace6DCost(
    const Robot& robot, const int frame_id, 
    const std::shared_ptr<TimeVaryingTaskSpace6DRefBase>& ref) 
  : CostFunctionComponentBase(),
    frame_id_(frame_id),
    ref_(ref),
    q_6d_weight_(Eigen::VectorXd::Zero(6)), 
    qf_6d_weight_(Eigen::VectorXd::Zero(6)), 
    qi_6d_weight_(Eigen::VectorXd::Zero(6)) {
}


TimeVaryingTaskSpace6DCost::TimeVaryingTaskSpace6DCost()
  : CostFunctionComponentBase(),
    frame_id_(0),
    ref_(),
    q_6d_weight_(Eigen::VectorXd::Zero(6)), 
    qf_6d_weight_(Eigen::VectorXd::Zero(6)), 
    qi_6d_weight_(Eigen::VectorXd::Zero(6)) {
}


TimeVaryingTaskSpace6DCost::~TimeVaryingTaskSpace6DCost() {
}


void TimeVaryingTaskSpace6DCost::set_ref(
    const std::shared_ptr<TimeVaryingTaskSpace6DRefBase>& ref) {
  ref_ = ref;
}


void TimeVaryingTaskSpace6DCost::set_q_weight(
    const Eigen::Vector3d& position_weight, 
    const Eigen::Vector3d& rotation_weight) {
  q_6d_weight_.template head<3>() = rotation_weight;
  q_6d_weight_.template tail<3>() = position_weight;
}


void TimeVaryingTaskSpace6DCost::set_qf_weight(
    const Eigen::Vector3d& position_weight, 
    const Eigen::Vector3d& rotation_weight) {
  qf_6d_weight_.template head<3>() = rotation_weight;
  qf_6d_weight_.template tail<3>() = position_weight;
}


void TimeVaryingTaskSpace6DCost::set_qi_weight(
    const Eigen::Vector3d& position_weight, 
    const Eigen::Vector3d& rotation_weight) {
  qi_6d_weight_.template head<3>() = rotation_weight;
  qi_6d_weight_.template tail<3>() = position_weight;
}
 

bool TimeVaryingTaskSpace6DCost::useKinematics() const {
  return true;
}


double TimeVaryingTaskSpace6DCost::evalStageCost(
    Robot& robot, const ContactStatus& contact_status, CostFunctionData& data, 
    const double t, const double dt, const SplitSolution& s) const {
  if (ref_->isActive(t)) {
    double l = 0;
    ref_->update_SE3_ref(t, data.SE3_ref);
    data.SE3_ref_inv = data.SE3_ref.inverse();
    data.diff_SE3 = data.SE3_ref_inv * robot.framePlacement(frame_id_);
    data.diff_6d = pinocchio::log6(data.diff_SE3).toVector();
    l += (q_6d_weight_.array()*data.diff_6d.array()*data.diff_6d.array()).sum();
    return 0.5 * dt * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingTaskSpace6DCost::evalStageCostDerivatives(
    Robot& robot, const ContactStatus& contact_status, CostFunctionData& data, 
    const double t, const double dt, const SplitSolution& s, 
    SplitKKTResidual& kkt_residual) const {
  if (ref_->isActive(t)) {
    data.J_66.setZero();
    pinocchio::Jlog6(data.diff_SE3, data.J_66);
    data.J_6d.setZero();
    robot.getFrameJacobian(frame_id_, data.J_6d);
    data.JJ_6d.noalias() = data.J_66 * data.J_6d;
    kkt_residual.lq().noalias() 
        += dt * data.JJ_6d.transpose() * q_6d_weight_.asDiagonal() 
                                        * data.diff_6d;
  }
}


void TimeVaryingTaskSpace6DCost::evalStageCostHessian(
    Robot& robot, const ContactStatus& contact_status, CostFunctionData& data, 
    const double t, const double dt, const SplitSolution& s, 
    SplitKKTMatrix& kkt_matrix) const {
  if (ref_->isActive(t)) {
    kkt_matrix.Qqq().noalias()
        += dt * data.JJ_6d.transpose() * q_6d_weight_.asDiagonal() * data.JJ_6d;
  }
}


double TimeVaryingTaskSpace6DCost::evalTerminalCost(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s) const {
  if (ref_->isActive(t)) {
    double l = 0;
    ref_->update_SE3_ref(t, data.SE3_ref);
    data.SE3_ref_inv = data.SE3_ref.inverse();
    data.diff_SE3 = data.SE3_ref_inv * robot.framePlacement(frame_id_);
    data.diff_6d = pinocchio::log6(data.diff_SE3).toVector();
    l += (qf_6d_weight_.array()*data.diff_6d.array()*data.diff_6d.array()).sum();
    return 0.5 * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingTaskSpace6DCost::evalTerminalCostDerivatives(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  if (ref_->isActive(t)) {
    data.J_66.setZero();
    pinocchio::Jlog6(data.diff_SE3, data.J_66);
    data.J_6d.setZero();
    robot.getFrameJacobian(frame_id_, data.J_6d);
    data.JJ_6d.noalias() = data.J_66 * data.J_6d;
    kkt_residual.lq().noalias() 
        += data.JJ_6d.transpose() * qf_6d_weight_.asDiagonal() * data.diff_6d;
  }
}


void TimeVaryingTaskSpace6DCost::evalTerminalCostHessian(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix) const {
  if (ref_->isActive(t)) {
    kkt_matrix.Qqq().noalias()
        += data.JJ_6d.transpose() * qf_6d_weight_.asDiagonal() * data.JJ_6d;
  }
}


double TimeVaryingTaskSpace6DCost::evalImpulseCost(
    Robot& robot, const ImpulseStatus& impulse_status, CostFunctionData& data, 
    const double t, const ImpulseSplitSolution& s) const {
  if (ref_->isActive(t)) {
    double l = 0;
    ref_->update_SE3_ref(t, data.SE3_ref);
    data.SE3_ref_inv = data.SE3_ref.inverse();
    data.diff_SE3 = data.SE3_ref_inv * robot.framePlacement(frame_id_);
    data.diff_6d = pinocchio::log6(data.diff_SE3).toVector();
    l += (qi_6d_weight_.array()*data.diff_6d.array()*data.diff_6d.array()).sum();
    return 0.5 * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingTaskSpace6DCost::evalImpulseCostDerivatives(
    Robot& robot, const ImpulseStatus& impulse_status, CostFunctionData& data, 
    const double t, const ImpulseSplitSolution& s, 
    ImpulseSplitKKTResidual& kkt_residual) const {
  if (ref_->isActive(t)) {
    data.J_66.setZero();
    pinocchio::Jlog6(data.diff_SE3, data.J_66);
    data.J_6d.setZero();
    robot.getFrameJacobian(frame_id_, data.J_6d);
    data.JJ_6d.noalias() = data.J_66 * data.J_6d;
    kkt_residual.lq().noalias() 
        += data.JJ_6d.transpose() * qi_6d_weight_.asDiagonal() * data.diff_6d;
  }
}


void TimeVaryingTaskSpace6DCost::evalImpulseCostHessian(
    Robot& robot, const ImpulseStatus& impulse_status, CostFunctionData& data, 
    const double t, const ImpulseSplitSolution& s, 
    ImpulseSplitKKTMatrix& kkt_matrix) const {
  if (ref_->isActive(t)) {
    kkt_matrix.Qqq().noalias()
        += data.JJ_6d.transpose() * qi_6d_weight_.asDiagonal() * data.JJ_6d;
  }
}

} // namespace robotoc