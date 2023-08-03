# ESPHome: Dynamic 'on-time' component

## Overview
Basically, it functions similar to
[time.on_time](https://esphome.io/components/time/index.html#on-time-trigger)
trigger, but allows changing the schedule dynamically.

## Schedule aspects supported

* Hour and minute
* Days of week (multiple ones are fine)
* Second is internally set to zero
* All days of months are enabled internally
* All months are enabled internally

## Usage

The ESPHome configuration should consider following components:
- [Number](https://esphome.io/index.html#number-components) to fetch hours and minutes from
- [Switch](https://esphome.io/index.html#switch-components) to fetch days of week from

The component then updates its schedule dynamically as soon as the components
below change their values.

Multiple instances of the component are supported.

### Configuration variables

* **rtc** (*Required*, [Time](https://esphome.io/components/time/index.html)): Time component to attach the schedule to
* **hour** (*Required*, [Number](https://esphome.io/index.html#number-components)): Component to get scheduled hour from
* **minute** (*Required*, [Number](https://esphome.io/index.html#number-components)): Component to get scheduled minute from
* **mon** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Component indicating the schedule is enabled for Monday
* **tue** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Similarly but for Tuesday
* **wed** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Similarly but for Wednesday
* **thu** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Similarly but for Thursday
* **fri** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Similarly but for Friday
* **sat** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Similarly but for Saturday
* **sun** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Similarly but for Sunday
* **disabled** (*Required*, [Switch](https://esphome.io/index.html#switch-components)): Component to disable scheduled actions
* **on_time** (*Requred*, [Automation](https://esphome.io/guides/automations.html#automation)) Automation to run when schedule triggers

## Example

The code below triggers sprinkler controller for full cycle as per schedule.
The HomeAssistant UI will present switches for each particular day of week, and
two number inputs for hour and minute.

```
external_components:
  source: github://hostcc/esphome-component-dynamic-on-time

switch:
  - platform: template
    id: lawn_sprinklers_mon
    name: "Lawn sprinklers: Mon"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
  - platform: template
    id: lawn_sprinklers_tue
    name: "Lawn sprinklers: Tue"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
  - platform: template
    id: lawn_sprinklers_wed
    name: "Lawn sprinklers: Wed"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
  - platform: template
    id: lawn_sprinklers_thu
    name: "Lawn sprinklers: Thu"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
  - platform: template
    id: lawn_sprinklers_fri
    name: "Lawn sprinklers: Fri"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
  - platform: template
    id: lawn_sprinklers_sat
    name: "Lawn sprinklers: Sat"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
  - platform: template
    id: lawn_sprinklers_sun
    name: "Lawn sprinklers: Sun"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
  - platform: template
    id: lawn_sprinklers_disabled
    name: "Lawn sprinklers: Disable"
    optimistic: true
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config

number:
  - platform: template
    id: lawn_sprinklers_hour
    name: "Lawn sprinklers: Hour"
    entity_category: config
    optimistic: true
    restore_value: true
    initial_value: 0
    min_value: 0
    max_value: 23
    step: 1
    mode: box
  - platform: template
    name: "Lawn sprinklers: Minute"
    id: lawn_sprinklers_minute
    entity_category: config
    optimistic: true
    restore_value: true
    initial_value: 0
    min_value: 0
    max_value: 59
    step: 1
    mode: box

time:
  - platform: homeassistant
    id: homeassistant_time

dynamic_on_time:
  - id: lawn_schedule
    rtc: homeassistant_time
    hour: lawn_sprinklers_hour
    minute: lawn_sprinklers_minute
    mon: lawn_sprinklers_mon
    tue: lawn_sprinklers_tue
    wed: lawn_sprinklers_wed
    thu: lawn_sprinklers_thu
    fri: lawn_sprinklers_fri
    sat: lawn_sprinklers_sat
    sun: lawn_sprinklers_sun
    disabled: lawn_sprinklers_disabled
    on_time:
      - logger.log:
          format: 'schedule: Starting full sprinkler cycle'
          tag: lawn_sprinklers
          level: 'INFO'
      - sprinkler.start_full_cycle: lawn_sprinklers
```
