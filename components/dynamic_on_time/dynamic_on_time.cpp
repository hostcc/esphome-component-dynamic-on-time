// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Ilia Sotnikov

#include "dynamic_on_time.h"  // NOLINT(build/include_subdir)
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace dynamic_on_time {

static const char *tag = "dynamic_on_time";

DynamicOnTime::DynamicOnTime(
  time::RealTimeClock *rtc,
  number::Number *hour,
  number::Number *minute,
  switch_::Switch *mon,
  switch_::Switch *tue,
  switch_::Switch *wed,
  switch_::Switch *thu,
  switch_::Switch *fri,
  switch_::Switch *sat,
  switch_::Switch *sun,
  std::vector<esphome::Action<> *> actions):
    rtc_(rtc),
    hour_(hour), minute_(minute),
    mon_(mon), tue_(tue), wed_(wed), thu_(thu), fri_(fri), sat_(sat),
    sun_(sun), actions_(actions) {}

std::vector<uint8_t> DynamicOnTime::flags_to_days_of_week_(
  bool mon, bool tue, bool wed, bool thu, bool fri, bool sat, bool sun
) {
  std::vector<uint8_t> days_of_week = { 1, 2, 3, 4, 5, 6, 7 };
  std::vector<bool> flags = { sun, mon, tue, wed, thu, fri, sat };

  // https://en.cppreference.com/w/cpp/algorithm/remove,
  // https://en.wikipedia.org/wiki/Erase%E2%80%93remove_idiom
  days_of_week.erase(
    std::remove_if(
      std::begin(days_of_week), std::end(days_of_week),
      [&](uint8_t& arg) { return !flags[&arg - days_of_week.data()]; }),
    days_of_week.end());

  return days_of_week;
}

void DynamicOnTime::setup() {
  for (auto *comp : std::vector<esphome::EntityBase *>{
    this->hour_,
    this->minute_,
    this->mon_, this->tue_, this->wed_, this->thu_, this->fri_, this->sat_,
    this->sun_
  }) {
    // `Switch` and `Number` has no common base class having
    // `add_on_state_callback` hence iterating over `EntityBase` casting it to
    // one of types having it
    reinterpret_cast<esphome::switch_::Switch *>(comp)->add_on_state_callback(
      [this](float value
    ) {
      this->update_schedule_();
    });
  }

  // Update the schedule initially
  this->update_schedule_();
}

void DynamicOnTime::update_schedule_() {
  bool to_register = false;

  assert(this->rtc_ != nullptr);
  if (this->trigger_ != nullptr) {
    // 'placement new', https://en.cppreference.com/w/cpp/language/new
    this->trigger_->~CronTrigger();
    new (this->trigger_) time::CronTrigger(this->rtc_);
  } else {
    to_register = true;
    this->trigger_ = new time::CronTrigger(this->rtc_);
  }

  // Automation
  if (this->automation_ != nullptr)
    delete this->automation_;

  this->automation_ = new Automation<>(this->trigger_);
  this->automation_->add_actions(this->actions_);

  this->trigger_->add_second(0);
  for (uint8_t i = 1; i <= 31; i++)
    this->trigger_->add_day_of_month(i);
  for (uint8_t i = 1; i <= 12; i++)
    this->trigger_->add_month(i);

  this->trigger_->add_hour(static_cast<uint8_t>(this->hour_->state));
  this->trigger_->add_minute(static_cast<uint8_t>(this->minute_->state));

  std::vector<uint8_t> days_of_week = this->flags_to_days_of_week_(
    this->mon_->state, this->tue_->state, this->wed_->state,
    this->thu_->state, this->fri_->state, this->sat_->state,
    this->sun_->state);
  this->trigger_->add_days_of_week(days_of_week);

  ESP_LOGCONFIG(
    tag, "Cron trigger details (updated: %s)",
    to_register ? "no": "yes");
  ESP_LOGCONFIG(tag, "Hour: %.0f", this->hour_->state);
  ESP_LOGCONFIG(tag, "Minute: %.0f", this->minute_->state);
  ESP_LOGCONFIG(tag, "Mon: %s", this->mon_->state ? "Yes": "No");
  ESP_LOGCONFIG(tag, "Tue: %s", this->tue_->state ? "Yes": "No");
  ESP_LOGCONFIG(tag, "Wed: %s", this->wed_->state ? "Yes": "No");
  ESP_LOGCONFIG(tag, "Thu: %s", this->thu_->state ? "Yes": "No");
  ESP_LOGCONFIG(tag, "Fri: %s", this->fri_->state ? "Yes": "No");
  ESP_LOGCONFIG(tag, "Sat: %s", this->sat_->state ? "Yes": "No");
  ESP_LOGCONFIG(tag, "Sun: %s", this->sun_->state ? "Yes": "No");

  if (to_register) {
    App.register_component(this->trigger_);
  }
}

}  // namespace dynamic_on_time
}  // namespace esphome
