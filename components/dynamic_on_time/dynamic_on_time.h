// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Ilia Sotnikov

#pragma once
#include <vector>
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/time/automation.h"
#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace dynamic_on_time {

// The component directly inherits from CronTrigger to allow manipulating
// with its protected/private members. Despite the API might get changed in
// future, the implementation has better maintainability because of simpler
// logic.
class DynamicOnTime : public time::CronTrigger {
 public:
  explicit DynamicOnTime(
    time::RealTimeClock *, number::Number *, number::Number *,
    switch_::Switch *, switch_::Switch *, switch_::Switch *, switch_::Switch *,
    switch_::Switch *, switch_::Switch *, switch_::Switch *, switch_::Switch *);

  void dump_config() override;
  void setup() override;

  optional<ESPTime> get_next_schedule();

 protected:
  number::Number *hour_comp_;
  number::Number *minute_comp_;
  switch_::Switch *mon_comp_;
  switch_::Switch *tue_comp_;
  switch_::Switch *wed_comp_;
  switch_::Switch *thu_comp_;
  switch_::Switch *fri_comp_;
  switch_::Switch *sat_comp_;
  switch_::Switch *sun_comp_;
  switch_::Switch *disabled_comp_;
  std::vector<uint8_t> days_of_week_{};

  std::vector<uint8_t> flags_to_days_of_week_(
    bool, bool, bool, bool, bool, bool, bool);

  void update_schedule_();
  optional<ESPTime> next_schedule_{};

  void reset_trigger_();
};

}  // namespace dynamic_on_time
}  // namespace esphome
