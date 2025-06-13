#include "elevator.h"

#include <algorithm>
#include <stdexcept>

Elevator::Elevator(int starting_floor, double max_load, size_t total_floors,
                   ElevatorState initial_state)
    : m_current_floor(starting_floor),
      m_state(initial_state),
      m_current_load(0.0),
      m_max_load(max_load),
      m_pressed_buttons(total_floors + 1, false),
      m_idle_time(0),
      m_moving_time(0),
      m_floors_passed(0),
      m_total_cargo(0.0),
      m_max_load_reached(0.0),
      m_overloads_count(0) {
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

Elevator::Elevator() : Elevator(1, 1000.0, 10) {}

int Elevator::current_floor() const noexcept { return m_current_floor; }
ElevatorState Elevator::state() const noexcept { return m_state; }
double Elevator::current_load() const noexcept { return m_current_load; }
double Elevator::max_load() const noexcept { return m_max_load; }
const std::vector<bool>& Elevator::pressed_buttons() const noexcept {
  return m_pressed_buttons;
}

size_t Elevator::idle_time() const noexcept { return m_idle_time; }
size_t Elevator::moving_time() const noexcept { return m_moving_time; }
size_t Elevator::floors_passed() const noexcept { return m_floors_passed; }
double Elevator::total_cargo() const noexcept { return m_total_cargo; }
double Elevator::max_load_reached() const noexcept {
  return m_max_load_reached;
}
size_t Elevator::overloads_count() const noexcept { return m_overloads_count; }

bool Elevator::try_move_passenger_in(Passenger* p) {
  if (m_current_load + p->weight() > m_max_load) {
    p->set_overload_lift();
    ++m_overloads_count;
    return false;
  }

  for (auto const& person : m_passengers) {
    p->add_met_passenger(person);
  }
  m_passengers.push_back(p);
  m_pressed_buttons[p->target_floor()] = true;
  m_current_load += p->weight();
  m_total_cargo += p->weight();
  m_max_load_reached = std::max(m_current_load, m_max_load_reached);
  return true;
}

void Elevator::set_state(ElevatorState st) { m_state = st; }
