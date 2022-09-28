#ifndef ROBOTOC_CONSTRAINTS_DATA_HXX_
#define ROBOTOC_CONSTRAINTS_DATA_HXX_

#include "robotoc/constraints/constraints_data.hpp"


namespace robotoc {

inline ConstraintsData::ConstraintsData(const int time_stage) 
  : is_position_level_valid_(false), 
    is_velocity_level_valid_(false),
    is_acceleration_level_valid_(false),
    is_impulse_level_valid_(false) {
  setTimeStage(time_stage);
}


inline ConstraintsData::ConstraintsData()
  : is_position_level_valid_(false), 
    is_velocity_level_valid_(false),
    is_acceleration_level_valid_(false),
    is_impulse_level_valid_(false) {
}


inline ConstraintsData::~ConstraintsData() {
}


inline void ConstraintsData::setTimeStage(const int time_stage) {
  if (time_stage >= 2) {
    is_position_level_valid_     = true;
    is_velocity_level_valid_     = true;
    is_acceleration_level_valid_ = true;
    is_impulse_level_valid_      = false;
  }
  else if (time_stage == 1) {
    is_position_level_valid_     = false;
    is_velocity_level_valid_     = true;
    is_acceleration_level_valid_ = true;
    is_impulse_level_valid_      = false;
  }
  else if (time_stage == 0) {
    is_position_level_valid_     = false;
    is_velocity_level_valid_     = false;
    is_acceleration_level_valid_ = true;
    is_impulse_level_valid_      = false;
  }
  else if (time_stage <= -1) {
    is_position_level_valid_     = false;
    is_velocity_level_valid_     = false;
    is_acceleration_level_valid_ = false;
    is_impulse_level_valid_      = true;
  }
}


inline bool ConstraintsData::isPositionLevelValid() const {
  return is_position_level_valid_;
}


inline bool ConstraintsData::isVelocityLevelValid() const {
  return is_velocity_level_valid_;
}


inline bool ConstraintsData::isAccelerationLevelValid() const {
  return is_acceleration_level_valid_;
}


inline bool ConstraintsData::isImpulseLevelValid() const {
  return is_impulse_level_valid_;
}


inline void ConstraintsData::copySlackAndDual(const ConstraintsData& other) {
  if (isPositionLevelValid()) {
    const int size = position_level_data.size();
    for (int i=0; i<size; ++i) {
      position_level_data[i].copySlackAndDual(other.position_level_data[i]);
    }
  }
  if (isVelocityLevelValid()) {
    const int size = velocity_level_data.size();
    for (int i=0; i<size; ++i) {
      velocity_level_data[i].copySlackAndDual(other.velocity_level_data[i]);
    }
  }
  if (isAccelerationLevelValid()) {
    const int size = acceleration_level_data.size();
    for (int i=0; i<size; ++i) {
      acceleration_level_data[i].copySlackAndDual(other.acceleration_level_data[i]);
    }
  }
  if (isImpulseLevelValid()) {
    const int size = impulse_level_data.size();
    for (int i=0; i<size; ++i) {
      impulse_level_data[i].copySlackAndDual(other.impulse_level_data[i]);
    }
  }
}


inline double ConstraintsData::KKTError() const {
  double err = 0.0;
  if (isPositionLevelValid()) {
    for (const auto& data : position_level_data) {
      err += data.KKTError();
    }
  }
  if (isVelocityLevelValid()) {
    for (const auto& data : velocity_level_data) {
      err += data.KKTError();
    }
  }
  if (isAccelerationLevelValid()) {
    for (const auto& data : acceleration_level_data) {
      err += data.KKTError();
    }
  }
  if (isImpulseLevelValid()) {
    for (const auto& data : impulse_level_data) {
      err += data.KKTError();
    }
  }
  return err;
}


inline double ConstraintsData::logBarrier() const {
  double lb = 0.0;
  if (isPositionLevelValid()) {
    for (const auto& data : position_level_data) {
      lb += data.log_barrier;
    }
  }
  if (isVelocityLevelValid()) {
    for (const auto& data : velocity_level_data) {
      lb += data.log_barrier;
    }
  }
  if (isAccelerationLevelValid()) {
    for (const auto& data : acceleration_level_data) {
      lb += data.log_barrier;
    }
  }
  if (isImpulseLevelValid()) {
    for (const auto& data : impulse_level_data) {
      lb += data.log_barrier;
    }
  }
  return lb;
}


template <int p>
inline double ConstraintsData::constraintViolation() const {
  return primalFeasibility<p>();
}


template <int p>
inline double ConstraintsData::primalFeasibility() const {
  double vio = 0.0;
  if (isPositionLevelValid()) {
    for (const auto& data : position_level_data) {
      vio += data.template primalFeasibility<p>();
    }
  }
  if (isVelocityLevelValid()) {
    for (const auto& data : velocity_level_data) {
      vio += data.template primalFeasibility<p>();
    }
  }
  if (isAccelerationLevelValid()) {
    for (const auto& data : acceleration_level_data) {
      vio += data.template primalFeasibility<p>();
    }
  }
  if (isImpulseLevelValid()) {
    for (const auto& data : impulse_level_data) {
      vio += data.template primalFeasibility<p>();
    }
  }
  return vio;
}


template <int p>
inline double ConstraintsData::dualFeasibility() const {
  double vio = 0.0;
  if (isPositionLevelValid()) {
    for (const auto& data : position_level_data) {
      vio += data.template dualFeasibility<p>();
    }
  }
  if (isVelocityLevelValid()) {
    for (const auto& data : velocity_level_data) {
      vio += data.template dualFeasibility<p>();
    }
  }
  if (isAccelerationLevelValid()) {
    for (const auto& data : acceleration_level_data) {
      vio += data.template dualFeasibility<p>();
    }
  }
  if (isImpulseLevelValid()) {
    for (const auto& data : impulse_level_data) {
      vio += data.template dualFeasibility<p>();
    }
  }
  return vio;
}

} // namespace robotoc

#endif // ROBOTOC_CONSTRAINTS_DATA_XXH