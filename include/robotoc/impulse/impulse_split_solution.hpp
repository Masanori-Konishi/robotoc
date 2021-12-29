#ifndef ROBOTOC_IMPULSE_SPLIT_SOLUTION_HPP_ 
#define ROBOTOC_IMPULSE_SPLIT_SOLUTION_HPP_

#include <vector>
#include <iostream>

#include "Eigen/Core"

#include "robotoc/robot/robot.hpp"
#include "robotoc/robot/impulse_status.hpp"
#include "robotoc/impulse/impulse_split_direction.hpp"
#include "robotoc/ocp/split_solution.hpp"


namespace robotoc {

///
/// @class ImpulseSplitSolution
/// @brief Solution to the optimal control problem split into an impulse time stage. 
///
class ImpulseSplitSolution {
public:
  using Vector6d = Eigen::Matrix<double, 6, 1>;

  ///
  /// @brief Construct an impulse split solution.
  /// @param[in] robot Robot model. 
  ///
  ImpulseSplitSolution(const Robot& robot);

  ///
  /// @brief Default constructor. 
  ///
  ImpulseSplitSolution();

  ///
  /// @brief Destructor. 
  ///
  ~ImpulseSplitSolution();

  ///
  /// @brief Default copy constructor. 
  ///
  ImpulseSplitSolution(const ImpulseSplitSolution&) = default;

  ///
  /// @brief Default copy assign operator. 
  ///
  ImpulseSplitSolution& operator=(const ImpulseSplitSolution&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  ImpulseSplitSolution(ImpulseSplitSolution&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  ImpulseSplitSolution& operator=(ImpulseSplitSolution&&) noexcept = default;

  ///
  /// @brief Set impulse status, i.e., set the dimension of the impulse.
  /// @param[in] impulse_status Impulse status.
  ///
  void setImpulseStatus(const ImpulseStatus& impulse_status);

  ///
  /// @brief Set impulse status, i.e., set the dimension of the impulse.
  /// @param[in] other Other impulse split solution.
  ///
  void setImpulseStatus(const ImpulseSplitSolution& other);

  ///
  /// @brief Configuration. Size is Robot::dimq().
  ///
  Eigen::VectorXd q;

  ///
  /// @brief Generalized velocity. Size is Robot::dimv().
  ///
  Eigen::VectorXd v;

  ///
  /// @brief Impulse change in the generalized velocity. 
  /// Size is Robot::dimv().
  ///
  Eigen::VectorXd dv;

  ///
  /// @brief Contact wrenches. Upper 3 elements are linear contact force
  /// and the lower 3 elements are the angular momentum.
  /// Size is Robot::maxNumContacts().
  ///
  std::vector<Vector6d> f;

  ///
  /// @brief Stack of the active impulse forces. Size is ImpulseStatus::dimf().
  /// @return Reference to the stack of the active impulse forces.
  ///
  Eigen::VectorBlock<Eigen::VectorXd> f_stack();

  ///
  /// @brief const version of ImpulseSplitSolution::f_stack().
  ///
  const Eigen::VectorBlock<const Eigen::VectorXd> f_stack() const;

  ///
  /// @brief Setss ImpulseSplitSolution::f_stack() from ImpulseSplitSolution::f.
  ///
  void set_f_stack();

  ///
  /// @brief Setss ImpulseSplitSolution::f from ImpulseSplitSolution::f_stack().
  ///
  void set_f_vector();

  ///
  /// @brief Lagrange multiplier w.r.t. the state equation w.r.t. q.
  /// Size is Robot::dimv().
  ///
  Eigen::VectorXd lmd;

  ///
  /// @brief Lagrange multiplier w.r.t. the state equation w.r.t. v.
  /// Size is Robot::dimv().
  ///
  Eigen::VectorXd gmm;

  ///
  /// @brief Lagrange multiplier w.r.t. impulse inverse dynamics. Size is 
  /// Robot::dimv().
  ///
  Eigen::VectorXd beta;

  ///
  /// @brief Lagrange multiplier w.r.t. the acceleration-level contact  
  /// constraint. Upper 3 elements are w.r.t. the linear contact acceleration
  /// and the lower 3 elements are w.r.t. the angular contact acceleration.
  /// Size is Robot::maxNumContacts().
  ///
  std::vector<Vector6d> mu;

  ///
  /// @brief Stack of the Lagrange multipliers w.r.t. the impulse velocity 
  /// constraints that are active at the current impulse status. Size is 
  /// ImpulseSplitSolution::dimf().
  /// @return Reference to the stack of the Lagrange multipliers w.r.t.
  /// the impulse velocity constraints.
  ///
  Eigen::VectorBlock<Eigen::VectorXd> mu_stack();

  ///
  /// @brief const version of ImpulseSplitSolution::mu_stack().
  ///
  const Eigen::VectorBlock<const Eigen::VectorXd> mu_stack() const;

  ///
  /// @brief Sets ImpulseSplitSolution::mu_stack() from ImpulseSplitSolution::mu. 
  ///
  void set_mu_stack();

  ///
  /// @brief Sets ImpulseSplitSolution::mu from ImpulseSplitSolution::mu_stack(). 
  ///
  void set_mu_vector();

  ///
  /// @brief Returns the dimension of the stack of the impulse forces at the 
  /// current impulse status.
  /// @return Dimension of the impulse forces.
  ///
  int dimi() const;

  ///
  /// @brief Return true if a impulse is active and false if not.
  /// @param[in] contact_index Index of a contact at impulse. 
  /// @return true if a Impulse is active and false if not. 
  ///
  bool isImpulseActive(const int contact_index) const;

  ///
  /// @brief Return activities of impulses.
  /// @return Activities of impulses. 
  ///
  std::vector<bool> isImpulseActive() const;

  ///
  /// @brief Integrates the solution based on step size and direction. 
  /// @param[in] robot Robot model.
  /// @param[in] step_size Step size.
  /// @param[in] d Split direction.
  ///
  void integrate(const Robot& robot, const double step_size, 
                 const ImpulseSplitDirection& d);

  ///
  /// @brief Copies the primal solution from another impulse solution. 
  /// @param[in] another Another impulse split solution.
  ///
  void copyPrimal(const ImpulseSplitSolution& another);

  ///
  /// @brief Copies the dual solution from another impulse solution. 
  /// @param[in] another Another impulse split solution.
  ///
  void copyDual(const ImpulseSplitSolution& another);

  ///
  /// @brief Return L-infinity Norm of the lagrange multipliers. Used in
  /// line search.
  ///
  double lagrangeMultiplierLinfNorm() const;

  ///
  /// @brief Return true if two ImpulseSplitSolution have the same value and  
  /// false if not. 
  /// @param[in] other Impulse split solution that is compared with this object.
  ///
  bool isApprox(const ImpulseSplitSolution& other) const;

  ///
  /// @brief Set each component vector by random value based on the current 
  /// impulse status. 
  /// @param[in] robot Robot model.
  ///
  void setRandom(const Robot& robot);

  ///
  /// @brief Set each component vector by random value. Impulse status is reset.
  /// @param[in] robot Robot model.
  /// @param[in] impulse_status Impulse status.
  ///
  void setRandom(const Robot& robot, const ImpulseStatus& impulse_status);

  ///
  /// @brief Generates impulse split solution filled randomly.
  /// @return Impulse split solution filled randomly.
  /// @param[in] robot Robot model. 
  ///
  static ImpulseSplitSolution Random(const Robot& robot);

  ///
  /// @brief Generates impulse split solution filled randomly.
  /// @return Impulse split solution filled randomly.
  /// @param[in] robot Robot model. 
  /// @param[in] impulse_status Impulse status.
  ///
  static ImpulseSplitSolution Random(const Robot& robot, 
                                     const ImpulseStatus& impulse_status);

  ///
  /// @brief Displays the impulse split solution onto a ostream.
  ///
  void disp(std::ostream& os) const;

  friend std::ostream& operator<<(std::ostream& os, 
                                  const ImpulseSplitSolution& s);

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  Eigen::VectorXd mu_stack_, f_stack_;
  std::vector<ContactType> contact_types_;
  std::vector<bool> is_impulse_active_;
  int dimi_, max_num_contacts_;

};

} // namespace robotoc 

#include "robotoc/impulse/impulse_split_solution.hxx"

#endif // ROBOTOC_IMPULSE_SPLIT_SOLUTION_HPP_ 