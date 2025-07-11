#include "elevator.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

Elevator::Elevator(size_t id, int starting_floor, double max_load,
                   size_t total_floors, ElevatorState initial_state)
    : m_current_floor(starting_floor),
      m_state(initial_state),
      m_current_load(0.0),
      m_max_load(max_load),
      m_pressed_buttons(total_floors + 1, false),
      m_id(id) {
  if (starting_floor < 1) {
    throw std::invalid_argument("Starting floor must be positive");
  }
  if (max_load <= 0) {
    throw std::invalid_argument("Max load must be positive");
  }
  if (total_floors == 0) {
    throw std::invalid_argument("Total floors must be positive");
  }
}

Elevator::Elevator() : Elevator(0, 1, 1000.0, 10) {}

size_t Elevator::current_floor() const noexcept { return m_current_floor; }
ElevatorState Elevator::state() const noexcept { return m_state; }
double Elevator::current_load() const noexcept { return m_current_load; }
double Elevator::max_load() const noexcept { return m_max_load; }
std::vector<bool> &Elevator::pressed_buttons() noexcept {
  return m_pressed_buttons;
}

size_t Elevator::idle_time() const noexcept { return m_idle_time; }
size_t Elevator::moving_time() const noexcept {
  return m_time_travel_ends - m_timestamp_when_last_state_set;
}
size_t Elevator::floors_passed() const noexcept { return m_floors_passed; }
double Elevator::total_cargo() const noexcept { return m_total_cargo; }
double Elevator::max_load_reached() const noexcept {
  return m_max_load_reached;
}
size_t Elevator::overloads_count() const noexcept { return m_overloads_count; }

bool Elevator::try_move_passenger_in(Passenger *p) {
  if (m_current_load + p->weight() > m_max_load) {
    p->set_overload_lift();
    ++m_overloads_count;
    return false;
  }

  for (auto const &person : m_passengers) {
    p->add_met_passenger(person);
  }
  m_passengers.push_back(p);
  m_pressed_buttons[p->target_floor()] = true;
  m_current_load += p->weight();
  m_total_cargo += p->weight();
  m_max_load_reached = std::max(m_current_load, m_max_load_reached);
  return true;
}

void Elevator::set_state(ElevatorState st, size_t current_time) {
  size_t action_time = current_time - m_timestamp_when_last_state_set;
  switch (m_state) {
    case ElevatorState::MovingDown:
    case ElevatorState::MovingUp:
      m_moving_time += action_time;
      calculate_moving_time(current_time);
      break;
    case ElevatorState::IdleOpen:
    case ElevatorState::IdleClosed:
      m_idle_time += action_time;
      break;
  }
  m_timestamp_when_last_state_set = current_time;

  m_state = st;
}

std::vector<Passenger *> &Elevator::passengers() { return m_passengers; }

void Elevator::move_passenger_out(std::vector<Passenger *>::iterator &it) {
  Passenger *passenger = *it;

  // Update elevator stats
  m_current_load -= passenger->weight();
  m_pressed_buttons[passenger->target_floor()] = false;

  it = m_passengers.erase(it);
}

void Elevator::calculate_moving_time(size_t current_time) {
  size_t moving_time =
      3 + static_cast<size_t>(std::floor(5 * (m_current_load / m_max_load)));
  size_t floors_to_pass = m_target_floor - m_current_floor;
  m_time_travel_ends = current_time + moving_time * floors_to_pass;
}

void Elevator::calculate_moving_time_with_interrupt(size_t current_time,
                                                    size_t new_target_floor) {
  if (m_state == ElevatorState::IdleClosed ||
      m_state == ElevatorState::IdleOpen) {
    m_target_floor = new_target_floor;
    calculate_moving_time(current_time);
    return;
  }

  size_t time_elapsed = current_time - m_timestamp_when_last_state_set;

  size_t total_floors_to_pass =
      abs(static_cast<int>(m_target_floor) - static_cast<int>(m_current_floor));
  double floors_per_time_unit =
      static_cast<double>(total_floors_to_pass) /
      (m_time_travel_ends - m_timestamp_when_last_state_set);
  auto floors_passed_approx =
      static_cast<size_t>(time_elapsed * floors_per_time_unit);

  size_t current_floor_approx = (m_state == ElevatorState::MovingUp)
                                    ? m_current_floor + floors_passed_approx
                                    : m_current_floor - floors_passed_approx;

  m_current_floor = current_floor_approx;
  m_target_floor = new_target_floor;
  calculate_moving_time(current_time);

  m_floors_passed += floors_passed_approx;
}

size_t Elevator::time_travel_ends() const { return m_time_travel_ends; }

void Elevator::set_target_floor(size_t floor) { m_target_floor = floor; }

size_t Elevator::id() const { return m_id; }

void Elevator::set_current_floor(size_t floor) { m_current_floor = floor; }

size_t Elevator::target_floor() const { return m_target_floor; }

void Elevator::set_floors_passed(size_t floors) { m_floors_passed = floors; }

size_t Elevator::elevator_aproximate_floor(size_t time) const {
  if (m_state == ElevatorState::IdleClosed ||
      m_state == ElevatorState::IdleOpen) {
    return m_current_floor;
  }

  // return m_current_floor;

  if (m_time_travel_ends == m_timestamp_when_last_state_set ||
      time <= m_timestamp_when_last_state_set) {
    return m_current_floor;
  }
  if (time >= m_time_travel_ends) {
    return m_target_floor;
  }

  double percentage =
      static_cast<double>(time - m_timestamp_when_last_state_set) /
      static_cast<double>(m_time_travel_ends - m_timestamp_when_last_state_set);

  double floor_diff = static_cast<double>(m_target_floor) -
                      static_cast<double>(m_current_floor);
  auto aproximate_floor =
      m_current_floor + static_cast<size_t>(floor_diff * percentage);

  return aproximate_floor;
}
