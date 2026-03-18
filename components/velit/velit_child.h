#pragma once

#include "velit_state.h"
#include "esphome/core/helpers.h"

namespace esphome::velit {

class VelitHub;

class VelitClient : public Parented<VelitHub> {
 public:
  virtual void on_velit_state(const VelitState &state) = 0;

 protected:
  friend class VelitHub;
};

}  // namespace esphome::velit
