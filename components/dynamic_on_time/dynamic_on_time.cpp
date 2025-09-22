// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Ilia Sotnikov

#include "dynamic_on_time.h"  // NOLINT(build/include_subdir)
#include <vector>
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace dynamic_on_time {

static const char *const tag = "dynamic_on_time";

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
  switch_::Switch *disabled):
    // Invoke base class constructor with RTC instance
    time::CronTrigger(rtc),
    hour_comp_(hour), minute_comp_(minute),
    mon_comp_(mon), tue_comp_(tue), wed_comp_(wed), thu_comp_(thu),
    fri_comp_(fri), sat_comp_(sat), sun_comp_(sun), disabled_comp_(disabled) {
}

void DynamicOnTime::setup() {
    ESP_LOGD(tag, "Setting up component");
    // Update the configuration initially, ensuring all entities are created
    // before a callback would be delivered to them
    this->update_schedule_();

    // Registering callbacks in `setup()` to ensure all components are created
    ESP_LOGD(tag, "Registering state callbacks");
    // The `Number` and `Switch` has no common base type with
    // `add_on_state_callback`, and solutions to properly cast to derived class
    // in single loop over vector of base class instances seemingly imply more
    // code than just two loops
    for (number::Number *comp : {this->hour_comp_, this->minute_comp_}) {
      comp->add_on_state_callback([this](float value) {
        ESP_LOGD(tag, "Number state changed, updating schedule");
        this->update_schedule_();
      });
    }

    for (switch_::Switch *comp : {
      this->mon_comp_, this->tue_comp_, this->wed_comp_, this->thu_comp_,
      this->fri_comp_, this->sat_comp_, this->sun_comp_, this->disabled_comp_
    }) {
      comp->add_on_state_callback([this](bool value) {
        ESP_LOGD(tag, "Switch state changed, updating schedule");
        this->update_schedule_();
      });
    }
}

std::vector<uint8_t> DynamicOnTime::flags_to_days_of_week_(
  bool mon, bool tue, bool wed, bool thu, bool fri, bool sat, bool sun
) {
  // Numeric representation for days of week (starts from Sun internally)
  std::vector<uint8_t> days_of_week = { 1, 2, 3, 4, 5, 6, 7 };
  std::vector<bool> flags = { sun, mon, tue, wed, thu, fri, sat };

  // Translate set of bool flags into vector of corresponding numeric
  // representation. This uses 'erase-remove' approach (
  // https://en.cppreference.com/w/cpp/algorithm/remove,
  // https://en.wikipedia.org/wiki/Erase%E2%80%93remove_idiom)
  days_of_week.erase(
    std::remove_if(
      std::begin(days_of_week), std::end(days_of_week),
      [&](uint8_t& arg) { return !flags[&arg - days_of_week.data()]; }),
    days_of_week.end());

  return days_of_week;
}

void DynamicOnTime::reset_trigger_() {
  ESP_LOGD(tag, "Resetting cron trigger configuration");

  // Clear all trigger configuration that might have been set previously
  // (except the automation)
  this->seconds_.reset();
  this->minutes_.reset();
  this->hours_.reset();
  this->days_of_month_.reset();
  this->months_.reset();
  this->days_of_week_.clear();
  // Ensure the CronTrigger runs the matching upon next loop iteration
  this->last_check_.reset();
}

void DynamicOnTime::update_schedule_() {
  ESP_LOGD(tag, "Updating schedule");

  // Clear any previous trigger configuration
  this->reset_trigger_();

  // No configuration is done if scheduled actions are disabled
  if (this->disabled_comp_->state) {
    ESP_LOGD(tag, "Scheduled actions are disabled");
    // Dump the config to show the disabled state
    this->dump_config();
    return;
  }

  // All remaining logic is active regardless of whether scheduled actions are
  // disabled, since callbacks from Switch/Number components being active still
  // need to be processed otherwise inputs will be lost

  // Set trigger to fire on zeroth second of configured time
  this->add_second(0);
  // Enable all days of months for the schedule
  for (uint8_t i = 1; i <= 31; i++)
    this->add_day_of_month(i);
  // Same but for months
  for (uint8_t i = 1; i <= 12; i++)
    this->add_month(i);
  // Configure hour/minute of the schedule from corresponding components' state
  this->add_hour(static_cast<uint8_t>(this->hour_comp_->state));
  this->add_minute(static_cast<uint8_t>(this->minute_comp_->state));
  // Similarly but for days of week translating set of components' state to
  // vector of numeric representation as `CrontTrigger::add_days_of_week()`
  // requires
  this->days_of_week_ = this->flags_to_days_of_week_(
    this->mon_comp_->state, this->tue_comp_->state, this->wed_comp_->state,
    this->thu_comp_->state, this->fri_comp_->state, this->sat_comp_->state,
    this->sun_comp_->state);
  this->add_days_of_week(this->days_of_week_);

  // Initiate updating the cached value for the next schedule
  this->next_schedule_.reset();

  // Log the configuration
  this->dump_config();
}

optional<ESPTime> DynamicOnTime::get_next_schedule() {
  if (this->disabled_comp_->state || this->days_of_week_.empty())
    return {};

  ESPTime now = this->rtc_->now();

  if (now < this->next_schedule_.value_or(now))
    return this->next_schedule_;

  ESP_LOGVV(tag, "Non-cached calculation of next schedule");

  // Calculate timestamp for the start of the week with time being hour/time of
  // the schedule
  time_t start_of_week = now.timestamp
    - (now.second + now.hour * 3600 + now.minute * 60 + now.day_of_week * 86400)
    + (3600 * static_cast<int>(this->hour_comp_->state)
    + 60 * static_cast<int>(this->minute_comp_->state));

  time_t next = 0, first = 0;
  for (auto next_day : this->days_of_week_) {
    // Calculate the timestamp for next day in schedule
    next = start_of_week + 86400 * next_day;
    // Capture timestamp for the first scheduled day
    if (!first)
      first = next;
    // Exit if timestamp corresponds of later date in the schedule found
    if (next > now.timestamp)
      break;
  }
  // No later date has been found, use the earlier of scheduled ones plus one
  // week
  if (next < now.timestamp)
    next = first + 7 * 86400;

  return this->next_schedule_ = ESPTime::from_epoch_local(next);
}

void DynamicOnTime::dump_config() {
  ESP_LOGCONFIG(tag, "Cron trigger details:");
  ESP_LOGCONFIG(tag, "Disabled: %s", ONOFF(this->disabled_comp_->state));
  ESP_LOGCONFIG(
    tag, "Hour (source: '%s'): %.0f",
    this->hour_comp_->get_name().c_str(), this->hour_comp_->state);
  ESP_LOGCONFIG(
    tag, "Minute (source: '%s'): %.0f",
    this->minute_comp_->get_name().c_str(), this->minute_comp_->state);
  ESP_LOGCONFIG(
    tag, "Mon (source: '%s'): %s",
    this->mon_comp_->get_name().c_str(), ONOFF(this->mon_comp_->state));
  ESP_LOGCONFIG(
    tag, "Tue (source: '%s'): %s",
    this->tue_comp_->get_name().c_str(), ONOFF(this->tue_comp_->state));
  ESP_LOGCONFIG(
    tag, "Wed (source: '%s'): %s",
    this->wed_comp_->get_name().c_str(), ONOFF(this->wed_comp_->state));
  ESP_LOGCONFIG(
    tag, "Thu (source: '%s'): %s",
    this->thu_comp_->get_name().c_str(), ONOFF(this->thu_comp_->state));
  ESP_LOGCONFIG(
    tag, "Fri (source: '%s'): %s",
    this->fri_comp_->get_name().c_str(), ONOFF(this->fri_comp_->state));
  ESP_LOGCONFIG(
    tag, "Sat (source: '%s'): %s",
    this->sat_comp_->get_name().c_str(), ONOFF(this->sat_comp_->state));
  ESP_LOGCONFIG(
    tag, "Sun (source: '%s'): %s",
    this->sun_comp_->get_name().c_str(), ONOFF(this->sun_comp_->state));

  auto schedule = this->get_next_schedule();
  if (schedule.has_value())
    ESP_LOGCONFIG(
      tag, "Next schedule: %s",
      schedule.value().strftime("%a %H:%M:%S %m/%d/%Y").c_str());
}

}  // namespace dynamic_on_time
}  // namespace esphome
