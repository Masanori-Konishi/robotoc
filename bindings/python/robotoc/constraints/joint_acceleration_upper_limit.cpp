#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "robotoc/constraints/joint_acceleration_upper_limit.hpp"


namespace robotoc {
namespace python {

namespace py = pybind11;

PYBIND11_MODULE(joint_acceleration_upper_limit, m) {
  py::class_<JointAccelerationUpperLimit, ConstraintComponentBase, 
             std::shared_ptr<JointAccelerationUpperLimit>>(m, "JointAccelerationUpperLimit")
    .def(py::init<const Robot&, const Eigen::VectorXd&, const double, const double>(),
         py::arg("robot"), py::arg("amax"), py::arg("barrier")=1.0e-04,
         py::arg("fraction_to_boundary_rule")=0.995);
}

} // namespace python
} // namespace robotoc