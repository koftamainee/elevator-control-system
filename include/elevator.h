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
  explicit Elevator(size_t id, int starting_floor, double max_load,
                    size_t total_floors,
                    ElevatorState initial_state = ElevatorState::IdleClosed);

  Elevator();

  size_t current_floor() const noexcept;
  ElevatorState state() const noexcept;
  double current_load() const noexcept;
  double max_load() const noexcept;
  std::vector<bool> &pressed_buttons() noexcept;

  size_t idle_time() const noexcept;
  size_t moving_time() const noexcept;
  size_t floors_passed() const noexcept;
  double total_cargo() const noexcept;
  double max_load_reached() const noexcept;
  size_t overloads_count() const noexcept;
  std::vector<Passenger *> &passengers();
  void calculate_moving_time(size_t current_time);
  size_t time_travel_ends() const;
  size_t id() const;
  void set_floors_passed(size_t floors);

  bool try_move_passenger_in(Passenger *p);
  void move_passenger_out(std::vector<Passenger *>::iterator &it);

  void set_state(ElevatorState st, size_t current_time);
  void set_target_floor(size_t floor);
  void set_current_floor(size_t floor);
  size_t target_floor() const;

  size_t elevator_aproximate_floor(size_t time) const;
  void calculate_moving_time_with_interrupt(size_t m_time, size_t floor);

 private:
  size_t m_id;
  size_t m_current_floor;
  ElevatorState m_state;
  double m_current_load;
  const double m_max_load;
  std::vector<bool> m_pressed_buttons;
  std::vector<Passenger *> m_passengers;
  size_t m_target_floor = 0;
  size_t m_timestamp_when_last_state_set =
      0;  // aka when the moving process starts
  size_t m_time_travel_ends = 0;

  size_t m_idle_time = 0;
  size_t m_moving_time = 0;
  size_t m_floors_passed = 0;
  double m_total_cargo = 0;
  double m_max_load_reached = 0;
  size_t m_overloads_count = 0;
};
