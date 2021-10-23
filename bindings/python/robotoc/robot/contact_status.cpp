#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "robotoc/robot/contact_status.hpp"

#include <iostream>


namespace robotoc {
namespace python {

namespace py = pybind11;

PYBIND11_MODULE(contact_status, m) {
  py::class_<ContactStatus>(m, "ContactStatus")
    .def(py::init<const int>())
    .def("max_point_contacts", &ContactStatus::maxPointContacts)
    .def("is_contact_active", 
          static_cast<bool (ContactStatus::*)(const int) const>(&ContactStatus::isContactActive))
    .def("is_contact_active", 
          static_cast<const std::vector<bool>& (ContactStatus::*)() const>(&ContactStatus::isContactActive))
    .def("activate_contact", &ContactStatus::activateContact)
    .def("deactivate_contact", &ContactStatus::deactivateContact)
    .def("activate_contacts", 
          static_cast<void (ContactStatus::*)(const std::vector<int>& contact_indices)>(&ContactStatus::activateContacts))
    .def("deactivate_contacts", 
          static_cast<void (ContactStatus::*)(const std::vector<int>& contact_indices)>(&ContactStatus::deactivateContacts))
    .def("activate_contacts", 
          static_cast<void (ContactStatus::*)()>(&ContactStatus::activateContacts))
    .def("deactivate_contacts", 
          static_cast<void (ContactStatus::*)()>(&ContactStatus::deactivateContacts))
    .def("set_contact_point", &ContactStatus::setContactPoint)
    .def("set_contact_points", &ContactStatus::setContactPoints)
    .def("contact_point", &ContactStatus::contactPoint)
    .def("contact_points", &ContactStatus::contactPoints)
    .def("__str__", [](const ContactStatus& self) {
        std::stringstream ss;
        ss << self;
        return ss.str();
      });
}

} // namespace python
} // namespace robotoc