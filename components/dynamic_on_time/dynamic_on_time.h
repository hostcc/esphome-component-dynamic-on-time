// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Ilia Sotnikov

#pragma once
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/time/automation.h"
#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace dynamic_on_time {

// Owns a CronTrigger and reconfigures it at runtime from Number/Switch
// entities. CronTrigger is final since ESPHome 2026.7.0, so schedule resets
// reconstruct the owned instance in place (placement-new) then re-apply
// add_* configuration via the public API.
class DynamicOnTime : public Component {
 public:
  explicit DynamicOnTime(
    time::RealTimeClock *, number::Number *, number::Number *,
    switch_::Switch *, switch_::Switch *, switch_::Switch *, switch_::Switch *,
    switch_::Switch *, switch_::Switch *, switch_::Switch *, switch_::Switch *);

  void dump_config() override;
  void setup() override;

  time::CronTrigger *get_cron_trigger() { return &this->cron_; }
  void set_automation(Automation<> *automation);

  optional<ESPTime> get_next_schedule();

 protected:
  time::CronTrigger cron_;
  time::RealTimeClock *rtc_;
  Automation<> *automation_{nullptr};

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
  std::vector<uint8_t> days_of_week_cache_{};

  std::vector<uint8_t> flags_to_days_of_week_(
    bool, bool, bool, bool, bool, bool, bool);

  void update_schedule_();
  optional<ESPTime> next_schedule_{};

  void reset_trigger_();
};

}  // namespace dynamic_on_time
}  // namespace esphome
