#include "robotoc/dynamics/contact_dynamics.hpp"

#include <cassert>


namespace robotoc {

ContactDynamics::ContactDynamics(const Robot& robot) 
  : data_(robot),
    has_floating_base_(robot.hasFloatingBase()),
    has_active_contacts_(false),
    dimv_(robot.dimv()),
    dimu_(robot.dimu()),
    dim_passive_(robot.dim_passive()) {
}


ContactDynamics::ContactDynamics() 
  : data_(),
    has_floating_base_(false),
    has_active_contacts_(false),
    dimv_(0),
    dimu_(0),
    dim_passive_(0) {
}


ContactDynamics::~ContactDynamics() {
}


void ContactDynamics::evalContactDynamics(
    Robot& robot, const ContactStatus& contact_status, const SplitSolution& s) {
  data_.setContactStatus(contact_status);
  has_active_contacts_ = contact_status.hasActiveContacts();
  robot.setContactForces(contact_status, s.f);
  robot.RNEA(s.q, s.v, s.a, data_.ID_full());
  data_.ID().noalias() -= s.u;
  robot.computeBaumgarteResidual(contact_status, data_.C());
}


void ContactDynamics::linearizeContactDynamics(
    Robot& robot, const ContactStatus& contact_status, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) { 
  evalContactDynamics(robot, contact_status, s);
  robot.RNEADerivatives(s.q, s.v, s.a, data_.dIDdq(), data_.dIDdv(), data_.dIDda);
  robot.computeBaumgarteDerivatives(contact_status, data_.dCdq(), data_.dCdv(), 
                                    data_.dCda());
  // augment inverse dynamics constraint
  kkt_residual.lq().noalias() += data_.dIDdq().transpose() * s.beta;
  kkt_residual.lv().noalias() += data_.dIDdv().transpose() * s.beta;
  kkt_residual.la.noalias()   += data_.dIDda.transpose() * s.beta;
  if (has_active_contacts_) {
    kkt_residual.lf().noalias() -= data_.dCda() * s.beta;
  }
  if (has_floating_base_) {
    // augment floating base constraint
    data_.lu_passive            = s.nu_passive;
    data_.lu_passive.noalias() -= s.beta.template head<kDimFloatingBase>(); 
    kkt_residual.lu.noalias()  -= s.beta.tail(robot.dimu()); 
  }
  else {
    kkt_residual.lu.noalias() -= s.beta; 
  }
  // augment acceleration-level contact constraint
  if (has_active_contacts_) {
    kkt_residual.lq().noalias() += data_.dCdq().transpose() * s.mu_stack();
    kkt_residual.lv().noalias() += data_.dCdv().transpose() * s.mu_stack();
    kkt_residual.la.noalias()   += data_.dCda().transpose() * s.mu_stack();
  }
}


void ContactDynamics::condenseContactDynamics(
    Robot& robot, const ContactStatus& contact_status, const double dt,
    SplitKKTMatrix& kkt_matrix, SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  const int dim_passive = robot.dim_passive();
  const int dimf = contact_status.dimf();
  robot.computeMJtJinv(data_.dIDda, data_.dCda(), data_.MJtJinv());
  data_.MJtJinv_dIDCdqv().noalias() = data_.MJtJinv() * data_.dIDCdqv();
  data_.MJtJinv_IDC().noalias()     = data_.MJtJinv() * data_.IDC();

  data_.Qafqv().topRows(dimv).noalias() 
      = (- kkt_matrix.Qaa.diagonal()).asDiagonal() 
          * data_.MJtJinv_dIDCdqv().topRows(dimv);
  data_.Qafqv().bottomRows(dimf).noalias() 
      = - kkt_matrix.Qff() * data_.MJtJinv_dIDCdqv().bottomRows(dimf);
  data_.Qafqv().bottomLeftCorner(dimf, dimv).noalias()
      -= kkt_matrix.Qqf().transpose();
  data_.Qafu_full().topRows(dimv).noalias() 
      = kkt_matrix.Qaa.diagonal().asDiagonal() 
          * data_.MJtJinv().topLeftCorner(dimv, dimv);
  data_.Qafu_full().bottomRows(dimf).noalias() 
      = kkt_matrix.Qff() * data_.MJtJinv().bottomLeftCorner(dimf, dimv);
  data_.la() = kkt_residual.la;
  data_.lf() = - kkt_residual.lf();
  data_.la().noalias() 
      -= kkt_matrix.Qaa.diagonal().asDiagonal() 
          * data_.MJtJinv_IDC().head(dimv);
  data_.lf().noalias() 
      -= kkt_matrix.Qff() * data_.MJtJinv_IDC().tail(dimf);

  kkt_matrix.Qxx.noalias() 
      -= data_.MJtJinv_dIDCdqv().transpose() * data_.Qafqv();
  kkt_matrix.Qxx.topRows(dimv).noalias() 
      += kkt_matrix.Qqf() * data_.MJtJinv_dIDCdqv().bottomRows(dimf);
  if (has_floating_base_) {
    data_.Qxu_passive.noalias() 
        = - data_.MJtJinv_dIDCdqv().transpose() * data_.Qafu_full().leftCols(dim_passive);
    data_.Qxu_passive.topRows(dimv).noalias()
        -= kkt_matrix.Qqf() * data_.MJtJinv().bottomLeftCorner(dimf, dimv).leftCols(dim_passive);
    kkt_matrix.Qxu.noalias() 
        -= data_.MJtJinv_dIDCdqv().transpose() * data_.Qafu_full().rightCols(dimu);
    kkt_matrix.Qxu.topRows(dimv).noalias()
        -= kkt_matrix.Qqf() * data_.MJtJinv().bottomLeftCorner(dimf, dimv).rightCols(dimu);
  }
  else {
    kkt_matrix.Qxu.noalias() 
        -= data_.MJtJinv_dIDCdqv().transpose() * data_.Qafu_full();
    kkt_matrix.Qxu.topRows(dimv).noalias()
        -= kkt_matrix.Qqf() * data_.MJtJinv().bottomLeftCorner(dimf, dimv);
  }
  kkt_residual.lx.noalias() 
      -= data_.MJtJinv_dIDCdqv().transpose() * data_.laf();
  kkt_residual.lq().noalias()
      += kkt_matrix.Qqf() * data_.MJtJinv_IDC().tail(dimf);

  if (has_floating_base_) {
    data_.Quu_passive_topRight.noalias() 
        = data_.MJtJinv().topRows(dim_passive) * data_.Qafu_full().rightCols(dimu);
    kkt_matrix.Quu.noalias() 
        += data_.MJtJinv().middleRows(dim_passive, dimu) * data_.Qafu_full().rightCols(dimu);
  }
  else {
    kkt_matrix.Quu.noalias() 
        += data_.MJtJinv().topRows(dimv) * data_.Qafu_full();
  }
  if (has_floating_base_) {
    data_.lu_passive.noalias() 
        += data_.MJtJinv().template topRows<kDimFloatingBase>() * data_.laf();
  }
  kkt_residual.lu.noalias() 
      += data_.MJtJinv().middleRows(dim_passive, dimu) * data_.laf();

  kkt_matrix.Fvq() = - dt * data_.MJtJinv_dIDCdqv().topLeftCorner(dimv, dimv);
  kkt_matrix.Fvv().noalias() 
        = - dt * data_.MJtJinv_dIDCdqv().topRightCorner(dimv, dimv) 
          + Eigen::MatrixXd::Identity(dimv, dimv);
  kkt_matrix.Fvu = dt * data_.MJtJinv().block(0, dim_passive, dimv, dimu);
  kkt_residual.Fv().noalias() -= dt * data_.MJtJinv_IDC().head(dimv);

  // STO sensitivities
  data_.ha() = kkt_matrix.ha;
  data_.hf() = - kkt_matrix.hf();
  kkt_residual.h -= data_.MJtJinv_IDC().dot(data_.haf()); 
  kkt_matrix.hx.noalias() -= data_.MJtJinv_dIDCdqv().transpose() * data_.haf();
  kkt_matrix.hq().noalias()
      += (1.0/dt) * kkt_matrix.Qqf() * data_.MJtJinv_IDC().tail(dimf);
  kkt_matrix.hu.noalias() 
      += data_.MJtJinv().middleRows(dim_passive, dimu) * data_.haf();
}


void ContactDynamics::condenseSwitchingConstraint(
    SwitchingConstraintJacobian& sc_jacobian,
    SwitchingConstraintResidual& sc_residual,
    SplitKKTMatrix& kkt_matrix) const {
  sc_jacobian.Phix().noalias() 
      -= sc_jacobian.Phia() * data_.MJtJinv_dIDCdqv().topRows(dimv_);
  sc_jacobian.Phiu().noalias()  
      = sc_jacobian.Phia() * data_.MJtJinv().block(0, dim_passive_, dimv_, dimu_);
  sc_jacobian.Phit().noalias() 
      -= sc_jacobian.Phia() * data_.MJtJinv_IDC().head(dimv_);
  sc_residual.P().noalias() 
      -= sc_jacobian.Phia() * data_.MJtJinv_IDC().head(dimv_);
}

} // namespace robotoc 