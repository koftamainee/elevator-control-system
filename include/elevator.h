#pragma once

#include <cstdint>
#include <vector>

#include "passenger.h"

enum class ElevatorState : std::uint8_t {
  IdleClosed,
  IdleOpen,
  MovingUp,
  MovingDown,
};

class Elevator final {
 public:
  explicit Elevator(int starting_floor, double max_load, size_t total_floors,
                    ElevatorState initial_state = ElevatorState::IdleClosed);

  Elevator();

  int current_floor() const noexcept;
  ElevatorState state() const noexcept;
  double current_load() const noexcept;
  double max_load() const noexcept;
  const std::vector<bool> &pressed_buttons() const noexcept;

  size_t idle_time() const noexcept;
  size_t moving_time() const noexcept;
  size_t floors_passed() const noexcept;
  double total_cargo() const noexcept;
  double max_load_reached() const noexcept;
  size_t overloads_count() const noexcept;

  bool try_move_passenger_in(Passenger *p);

  void set_state(ElevatorState st);

 private:
  int m_current_floor;
  ElevatorState m_state;
  double m_current_load;
  const double m_max_load;
  std::vector<bool> m_pressed_buttons;
  std::vector<Passenger *> m_passengers;

  size_t m_idle_time;
  size_t m_moving_time;
  size_t m_floors_passed;
  double m_total_cargo;
  double m_max_load_reached;
  size_t m_overloads_count;
};
