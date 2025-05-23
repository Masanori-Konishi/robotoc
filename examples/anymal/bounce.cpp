#include <string>
#include <memory>

#include "Eigen/Core"

#include "robotoc/solver/ocp_solver.hpp"
#include "robotoc/ocp/ocp.hpp"
#include "robotoc/robot/robot.hpp"
#include "robotoc/planner/contact_sequence.hpp"
#include "robotoc/cost/cost_function.hpp"
#include "robotoc/cost/configuration_space_cost.hpp"
#include "robotoc/cost/task_space_3d_cost.hpp"
#include "robotoc/cost/com_cost.hpp"
#include "robotoc/cost/periodic_swing_foot_ref.hpp"
#include "robotoc/cost/periodic_com_ref.hpp"
#include "robotoc/constraints/constraints.hpp"
#include "robotoc/constraints/joint_position_lower_limit.hpp"
#include "robotoc/constraints/joint_position_upper_limit.hpp"
#include "robotoc/constraints/joint_velocity_lower_limit.hpp"
#include "robotoc/constraints/joint_velocity_upper_limit.hpp"
#include "robotoc/constraints/joint_torques_lower_limit.hpp"
#include "robotoc/constraints/joint_torques_upper_limit.hpp"
#include "robotoc/constraints/friction_cone.hpp"
#include "robotoc/solver/solver_options.hpp"

#include "robotoc/utils/ocp_benchmarker.hpp"


int main(int argc, char *argv[]) {
  robotoc::RobotModelInfo model_info;
  model_info.urdf_path = "../anymal_b_simple_description/urdf/anymal.urdf";
  model_info.base_joint_type = robotoc::BaseJointType::FloatingBase;
  const double baumgarte_time_step = 0.04;
  model_info.point_contacts = {robotoc::ContactModelInfo("LF_FOOT", baumgarte_time_step),
                               robotoc::ContactModelInfo("LH_FOOT", baumgarte_time_step),
                               robotoc::ContactModelInfo("RF_FOOT", baumgarte_time_step),
                               robotoc::ContactModelInfo("RH_FOOT", baumgarte_time_step)};
  robotoc::Robot robot(model_info);

  const double dt = 0.02;
  const Eigen::Vector3d step_length = {0.275, 0, 0};
  const double step_height = 0.125;
  const double swing_time = 0.26;
  const double double_support_time = 0.04;
  const double t0 = 0.10;
  const int cycle = 3; 

  // Create the cost function
  auto cost = std::make_shared<robotoc::CostFunction>();
  Eigen::VectorXd q_standing(Eigen::VectorXd::Zero(robot.dimq()));
  q_standing << 0, 0, 0.4792, 0, 0, 0, 1, 
                -0.1,  0.7, -1.0, 
                -0.1, -0.7,  1.0, 
                 0.1,  0.7, -1.0, 
                 0.1, -0.7,  1.0;
  Eigen::VectorXd q_weight(Eigen::VectorXd::Zero(robot.dimv()));
  q_weight << 0, 0, 0, 250000, 250000, 250000, 
              0.0001, 0.0001, 0.0001, 
              0.0001, 0.0001, 0.0001,
              0.0001, 0.0001, 0.0001,
              0.0001, 0.0001, 0.0001;
  Eigen::VectorXd v_weight(Eigen::VectorXd::Zero(robot.dimv()));
  v_weight << 100, 100, 100, 100, 100, 100, 
              1, 1, 1, 
              1, 1, 1,
              1, 1, 1,
              1, 1, 1;
  Eigen::VectorXd u_weight = Eigen::VectorXd::Constant(robot.dimu(), 1e-01);
  Eigen::VectorXd q_weight_impact(Eigen::VectorXd::Zero(robot.dimv()));
  q_weight_impact << 1, 1, 1, 1, 1, 1,  
               100, 100, 100, 
               100, 100, 100,
               100, 100, 100,
               100, 100, 100;
  Eigen::VectorXd v_weight_impact = Eigen::VectorXd::Constant(robot.dimv(), 100);
  auto config_cost = std::make_shared<robotoc::ConfigurationSpaceCost>(robot);
  config_cost->set_q_ref(q_standing);
  config_cost->set_q_weight(q_weight);
  config_cost->set_q_weight_terminal(q_weight);
  config_cost->set_q_weight_impact(q_weight_impact);
  config_cost->set_v_weight(v_weight);
  config_cost->set_v_weight_terminal(v_weight);
  config_cost->set_v_weight_impact(v_weight_impact);
  config_cost->set_u_weight(u_weight);
  cost->add("config_cost", config_cost);

  robot.updateFrameKinematics(q_standing);
  const Eigen::Vector3d x3d0_LF = robot.framePosition("LF_FOOT");
  const Eigen::Vector3d x3d0_LH = robot.framePosition("LH_FOOT");
  const Eigen::Vector3d x3d0_RF = robot.framePosition("RF_FOOT");
  const Eigen::Vector3d x3d0_RH = robot.framePosition("RH_FOOT");
  const double LF_t0 = t0 + swing_time + double_support_time;
  const double LH_t0 = t0;
  const double RF_t0 = t0 + swing_time + double_support_time;
  const double RH_t0 = t0;
  auto LF_foot_ref = std::make_shared<robotoc::PeriodicSwingFootRef>(x3d0_LF, step_length, step_height, 
                                                                     LF_t0, swing_time, 
                                                                     swing_time+2*double_support_time, false);
  auto LH_foot_ref = std::make_shared<robotoc::PeriodicSwingFootRef>(x3d0_LH, step_length, step_height, 
                                                                     LH_t0, swing_time, 
                                                                     swing_time+2*double_support_time, false);
  auto RF_foot_ref = std::make_shared<robotoc::PeriodicSwingFootRef>(x3d0_RF, step_length, step_height, 
                                                                     RF_t0, swing_time, 
                                                                     swing_time+2*double_support_time, false);
  auto RH_foot_ref = std::make_shared<robotoc::PeriodicSwingFootRef>(x3d0_RH, step_length, step_height, 
                                                                     RH_t0, swing_time, 
                                                                     swing_time+2*double_support_time, false);
  auto LF_cost = std::make_shared<robotoc::TaskSpace3DCost>(robot, "LF_FOOT", LF_foot_ref);
  auto LH_cost = std::make_shared<robotoc::TaskSpace3DCost>(robot, "LH_FOOT", LH_foot_ref);
  auto RF_cost = std::make_shared<robotoc::TaskSpace3DCost>(robot, "RF_FOOT", RF_foot_ref);
  auto RH_cost = std::make_shared<robotoc::TaskSpace3DCost>(robot, "RH_FOOT", RH_foot_ref);
  const Eigen::Vector3d foot_track_weight = Eigen::Vector3d::Constant(1.0e06);
  LF_cost->set_weight(foot_track_weight);
  LH_cost->set_weight(foot_track_weight);
  RF_cost->set_weight(foot_track_weight);
  RH_cost->set_weight(foot_track_weight);
  cost->add("LF_cost", LF_cost);
  cost->add("LH_cost", LH_cost);
  cost->add("RF_cost", RF_cost);
  cost->add("RH_cost", RH_cost);

  const Eigen::Vector3d com_ref0 = robot.CoM();
  const Eigen::Vector3d vcom_ref = 0.5 * step_length / swing_time;
  auto com_ref = std::make_shared<robotoc::PeriodicCoMRef>(com_ref0, vcom_ref, t0, swing_time, 
                                                           double_support_time, false);
  auto com_cost = std::make_shared<robotoc::CoMCost>(robot, com_ref);
  com_cost->set_weight(Eigen::Vector3d::Constant(1.0e06));
  cost->add("com_cost", com_cost);

  // Create the constraints
  const double barrier_param = 1.0e-03;
  const double fraction_to_boundary_rule = 0.995;
  auto constraints          = std::make_shared<robotoc::Constraints>(barrier_param, fraction_to_boundary_rule);
  auto joint_position_lower = std::make_shared<robotoc::JointPositionLowerLimit>(robot);
  auto joint_position_upper = std::make_shared<robotoc::JointPositionUpperLimit>(robot);
  auto joint_velocity_lower = std::make_shared<robotoc::JointVelocityLowerLimit>(robot);
  auto joint_velocity_upper = std::make_shared<robotoc::JointVelocityUpperLimit>(robot);
  auto joint_torques_lower  = std::make_shared<robotoc::JointTorquesLowerLimit>(robot);
  auto joint_torques_upper  = std::make_shared<robotoc::JointTorquesUpperLimit>(robot);
  auto friction_cone        = std::make_shared<robotoc::FrictionCone>(robot);
  constraints->add("joint_position_lower", joint_position_lower);
  constraints->add("joint_position_upper", joint_position_upper);
  constraints->add("joint_velocity_lower", joint_velocity_lower);
  constraints->add("joint_velocity_upper", joint_velocity_upper);
  constraints->add("joint_torques_lower", joint_torques_lower);
  constraints->add("joint_torques_upper", joint_torques_upper);
  constraints->add("friction_cone", friction_cone);

  // Create the contact sequence
  auto contact_sequence = std::make_shared<robotoc::ContactSequence>(robot);
  const double mu = 0.7;
  const std::unordered_map<std::string, double> friction_coefficients = {{"LF_FOOT", mu}, 
                                                                         {"LH_FOOT", mu}, 
                                                                         {"RF_FOOT", mu}, 
                                                                         {"RH_FOOT", mu}};

  std::unordered_map<std::string, Eigen::Vector3d> contact_positions = {{"LF_FOOT", x3d0_LF}, 
                                                                        {"LH_FOOT", x3d0_LH}, 
                                                                        {"RF_FOOT", x3d0_RF}, 
                                                                        {"RH_FOOT", x3d0_RH}};

  auto contact_status_standing = robot.createContactStatus();
  contact_status_standing.activateContacts(std::vector<std::string>({"LF_FOOT", "LH_FOOT", "RF_FOOT", "RH_FOOT"}));
  contact_status_standing.setContactPlacements(contact_positions);
  contact_status_standing.setFrictionCoefficients(friction_coefficients);
  contact_sequence->init(contact_status_standing);

  auto contact_status_hip_swing = robot.createContactStatus();
  contact_status_hip_swing.activateContacts(std::vector<std::string>({"LF_FOOT", "RF_FOOT"}));
  contact_status_hip_swing.setContactPlacements(contact_positions);
  contact_status_hip_swing.setFrictionCoefficients(friction_coefficients);
  contact_sequence->push_back(contact_status_hip_swing, t0);

  contact_positions["LH_FOOT"].noalias() += step_length;
  contact_positions["RH_FOOT"].noalias() += step_length;
  contact_status_standing.setContactPlacements(contact_positions);
  contact_sequence->push_back(contact_status_standing, t0+swing_time);

  auto contact_status_front_swing = robot.createContactStatus();
  contact_status_front_swing.activateContacts(std::vector<std::string>({"LH_FOOT", "RH_FOOT"}));
  contact_status_front_swing.setContactPlacements(contact_positions);
  contact_status_front_swing.setFrictionCoefficients(friction_coefficients);
  contact_sequence->push_back(contact_status_front_swing, 
                              t0+swing_time+double_support_time);

  contact_positions["LF_FOOT"].noalias() += step_length;
  contact_positions["RF_FOOT"].noalias() += step_length;
  contact_status_standing.setContactPlacements(contact_positions);
  contact_sequence->push_back(contact_status_standing, 
                              t0+2*swing_time+double_support_time);

  for (int i=1; i<cycle; ++i) {
    const double t1 = t0 + i*(2*swing_time+2*double_support_time);
    contact_status_hip_swing.setContactPlacements(contact_positions);
    contact_sequence->push_back(contact_status_hip_swing, t1);

    contact_positions["LH_FOOT"].noalias() += step_length;
    contact_positions["RH_FOOT"].noalias() += step_length;
    contact_status_standing.setContactPlacements(contact_positions);
    contact_sequence->push_back(contact_status_standing, t1+swing_time);

    contact_status_front_swing.setContactPlacements(contact_positions);
    contact_sequence->push_back(contact_status_front_swing, 
                                t1+swing_time+double_support_time);

    contact_positions["LF_FOOT"].noalias() += step_length;
    contact_positions["RF_FOOT"].noalias() += step_length;
    contact_status_standing.setContactPlacements(contact_positions);
    contact_sequence->push_back(contact_status_standing, 
                                t1+2*swing_time+double_support_time);
  }

  // you can check the contact sequence via
  // std::cout << contact_sequence << std::endl;

  // Create the OCP solver.
  const double T = t0 + cycle*(2*double_support_time+2*swing_time);
  const int N = T / dt; 
  robotoc::OCP ocp(robot, cost, constraints, contact_sequence, T, N);
  auto solver_options = robotoc::SolverOptions();
  solver_options.nthreads = 4;
  robotoc::OCPSolver ocp_solver(ocp, solver_options);

  // Initial time and initial state
  const double t = 0;
  const Eigen::VectorXd q(q_standing);
  const Eigen::VectorXd v(Eigen::VectorXd::Zero(robot.dimv()));

  // Solves the OCP.
  ocp_solver.discretize(t);
  ocp_solver.setSolution("q", q);
  ocp_solver.setSolution("v", v);
  Eigen::Vector3d f_init;
  f_init << 0, 0, 0.25*robot.totalWeight();
  ocp_solver.setSolution("f", f_init);
  ocp_solver.initConstraints();
  std::cout << "Initial KKT error: " << ocp_solver.KKTError(t, q, v) << std::endl;
  ocp_solver.solve(t, q, v);
  std::cout << "KKT error after convergence: " << ocp_solver.KKTError(t, q, v) << std::endl;
  std::cout << ocp_solver.getSolverStatistics() << std::endl;
  // const int num_iteration = 10000;
  // robotoc::benchmark::CPUTime(ocp_solver, t, q, v, num_iteration);

  return 0;
}