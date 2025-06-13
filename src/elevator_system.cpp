#include "elevator_system.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include "elevator.h"

ElevatorSystem::ElevatorSystem(std::vector<Elevator> elevators,
                               size_t floors_count, logger *log)
    : m_elevators(std::move(elevators)),
      m_floors_count(floors_count),
      m_elevators_count(elevators.size()),
      log(log),
      m_waiting_passengers_by_floor(floors_count + 1),
      m_pending_lift_calls(floors_count + 1) {}

ElevatorSystem &ElevatorSystem::model(std::string const &input_file) {
  parse_passengers_file(input_file);

  while (m_time < 1000) {
    arrive_passengers(m_time);
    for (int i = 1; i < m_waiting_passengers_by_floor.size(); ++i) {
      auto &pas_list = m_waiting_passengers_by_floor[i];
      if (pas_list.size() > 0 &&
          !m_floors_already_called_elevator.contains(i)) {
        Elevator *e = calculate_most_suitable_elevator(i);
        interrupt_elevator(e, i);
        m_floors_already_called_elevator.insert(i);
      }
    }
    for (auto &e : m_elevators) {
      if (e.time_travel_ends() == m_time && e.time_travel_ends() != 0 &&
          e.target_floor() != 0) {
        e.set_current_floor(e.target_floor());
        e.set_target_floor(0);
        process_floor_arival(e.current_floor(), &e);
      }
      // if (e.state() == ElevatorState::IdleClosed ||
      //     e.state() == ElevatorState::IdleOpen) {
      //   calculate_next_elevator_target(e.current_floor(), &e);
      // }
    }

    ++m_time;
  }

  std::cout << test_passengers_appeared_on_starting_floors << ", "
            << test_pasengers_succesfully_moved_to_dest << std::endl;

  std::cout << "passengers in elevators:\n";
  for (auto &e : m_elevators) {
    std::cout << ((e.state() == ElevatorState::IdleOpen ||
                   e.state() == ElevatorState::IdleClosed)
                      ? "idle"
                      : "moving")
              << std::endl;
    std::cout << e.passengers().size() << ": ";
    if (e.passengers().size() > 0) {
      for (auto &pas : e.passengers()) {
        std::cout << pas->target_floor() << " ";
      }
      std::cout << ", pressed_buttons: ";
      for (int i = 0; i < e.pressed_buttons().size(); ++i) {
        if (e.pressed_buttons()[i]) {
          std::cout << i << " ";
        }
      }
      std::cout << std::endl;
    }
  }

  return *this;
}

ElevatorSystem &ElevatorSystem::print_results(
    std::string const &passengers_file_path,
    std::string const &elevators_file_path) {
  return *this;
}

void ElevatorSystem::parse_passengers_file(std::string const &file) {
  std::ifstream fin(file);
  if (!fin.is_open()) {
    std::string const error_message =
        "Failed to open configuration file: " + file;

    error_with_guard(error_message);
    throw std::runtime_error(error_message);
  }

  size_t id = 0;
  double weight = 0;
  size_t current_floor = 0;
  std::string time;
  size_t target_floor = 0;

  while (fin >> id >> weight >> current_floor >> time >> target_floor) {
    size_t time_numeric = time_to_numerical(time);

    if (current_floor > m_floors_count || target_floor > m_floors_count) {
      std::string const error_message =
          "Invalid floor number for passenger " + std::to_string(id) +
          ": current_floor=" + std::to_string(current_floor) +
          ", target_floor=" + std::to_string(target_floor) +
          " (building has only " + std::to_string(m_floors_count) + " floors)";

      error_with_guard(error_message);
      throw std::runtime_error(error_message);
    }

    auto [it, inserted] = m_passengers.emplace(
        id, Passenger(id, time_numeric, current_floor, target_floor, weight));

    if (inserted) {
      ++m_remaining_passengers;
      information_with_guard("Passenger #" + std::to_string(id) + " | " +
                             std::to_string(weight) + " kg" + " | " + time +
                             " | floor " + std::to_string(current_floor) +
                             " → floor " + std::to_string(target_floor));

      m_time_index.emplace(time_numeric, &it->second);
    }
  }

  if (fin.bad()) {
    std::string const error_message = "Failed to read from file: " + file;
    error_with_guard(error_message);
    throw std::runtime_error(error_message);
  }
}

size_t ElevatorSystem::time_to_numerical(std::string const &time) const {
  size_t colon_pos = time.find(':');
  if (colon_pos == std::string::npos) {
    std::string const error_message =
        "Invalid time format for '" + time + "'. Expected 'hh:mm'";
    error_with_guard(error_message);
    throw std::runtime_error(error_message);
  }

  std::string hours_str = time.substr(0, colon_pos);
  std::string minutes_str = time.substr(colon_pos + 1);

  size_t hours = std::stoull(hours_str);
  size_t minutes = std::stoull(minutes_str);
  if (minutes >= 60) {
    std::string const error_message = "Invalid minutes value " + minutes_str +
                                      " in '" + time + "'. Must be < 60";
    error_with_guard(error_message);
    throw std::runtime_error(error_message);
  }

  size_t time_numerical = (hours * 60) + minutes;
  information_with_guard("Parsed time " + time +
                         " to numerical: " + std::to_string(time_numerical));
  return time_numerical;
}

void ElevatorSystem::process_floor_arival(size_t floor, Elevator *elevator) {
  elevator->set_state(ElevatorState::IdleOpen, m_time);
  elevator->pressed_buttons()[floor] = false;
  elevator->set_current_floor(elevator->target_floor());
  elevator->set_target_floor(0);
  if (m_floors_already_called_elevator.contains(floor)) {
    m_floors_already_called_elevator.erase(floor);
  }
  information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                         std::to_string(elevator->id()) + " arrived at floor " +
                         std::to_string(floor));

  process_passengers_deboarding(floor, elevator);

  move_passengers_from_floor_to_elevator(floor, elevator);

  calculate_next_elevator_target(floor, elevator);
}

void ElevatorSystem::process_passengers_deboarding(size_t floor,
                                                   Elevator *elevator) {
  auto &passengers = elevator->passengers();
  auto it = passengers.begin();
  while (it != passengers.end()) {
    Passenger *next_passenger = *it;
    if (next_passenger->target_floor() == floor) {
      next_passenger->set_deboarding_time(m_time);
      elevator->move_passenger_out(it);  // updates iterator
      information_with_guard("[" + std::to_string(m_time) + "] Passenger #" +
                             std::to_string(next_passenger->id()) +
                             " arrived at floor " + std::to_string(floor) +
                             " via elevator #" +
                             std::to_string(elevator->id()));
      --m_remaining_passengers;
      ++test_pasengers_succesfully_moved_to_dest;
    } else {
      ++it;
    }
  }
}

void ElevatorSystem::move_passengers_from_floor_to_elevator(
    size_t floor, Elevator *elevator) {
  auto &waiting_queue = m_waiting_passengers_by_floor[floor];

  auto it = waiting_queue.begin();

  while (it != waiting_queue.end()) {
    Passenger *next_passenger = *it;
    if (elevator->try_move_passenger_in(next_passenger)) {
      it = waiting_queue.erase(it);
      information_with_guard("[" + std::to_string(m_time) + "] Passenger #" +
                             std::to_string(next_passenger->id()) +
                             " entered elevator on floor " +
                             std::to_string(floor));
    } else {
      ++it;
    }
  }
}

void ElevatorSystem::calculate_next_elevator_target(size_t floor,
                                                    Elevator *elevator) const {
  const auto &buttons = elevator->pressed_buttons();

  if (elevator->state() == ElevatorState::MovingUp ||
      elevator->state() == ElevatorState::IdleClosed) {
    for (size_t f = floor + 1; f < buttons.size(); ++f) {
      if (buttons[f]) {
        information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                               std::to_string(elevator->id()) +
                               " continues MovingUp - next target floor " +
                               std::to_string(f));
        elevator->set_state(ElevatorState::MovingUp, m_time);
        return;
      }
    }

    for (size_t f = floor - 1; f != static_cast<size_t>(-1); --f) {
      if (buttons[f]) {
        information_with_guard(
            "[" + std::to_string(m_time) + "] Elevator #" +
            std::to_string(elevator->id()) +
            " changes direction to MovingDown - next target floor " +
            std::to_string(f));
        elevator->set_state(ElevatorState::MovingDown, m_time);
        return;
      }
    }

  } else if (elevator->state() == ElevatorState::MovingDown ||
             elevator->state() == ElevatorState::IdleClosed) {
    for (size_t f = floor - 1; f != static_cast<size_t>(-1); --f) {
      if (buttons[f]) {
        information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                               std::to_string(elevator->id()) +
                               " continues MovingDown - next target floor " +
                               std::to_string(f));
        elevator->set_state(ElevatorState::MovingDown, m_time);
        return;
      }
    }

    for (size_t f = floor + 1; f < buttons.size(); ++f) {
      if (buttons[f]) {
        information_with_guard(
            "[" + std::to_string(m_time) + "] Elevator #" +
            std::to_string(elevator->id()) +
            " changes direction to MovingUp - next target floor " +
            std::to_string(f));
        elevator->set_state(ElevatorState::MovingUp, m_time);
        return;
      }
    }
  }
  std::cout << " failed\n";

  information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                         std::to_string(elevator->id()) +
                         " started idleing (no buttons pressed)");
  elevator->set_state(ElevatorState::IdleClosed, m_time);
}

void ElevatorSystem::arrive_passengers(size_t current_time) {
  auto range_in_time = m_time_index.equal_range(current_time);
  for (auto it = range_in_time.first; it != range_in_time.second; ++it) {
    Passenger *p = it->second;
    m_waiting_passengers_by_floor[p->boarding_floor()].push_back(p);
    test_passengers_appeared_on_starting_floors++;
    information_with_guard("[" + std::to_string(m_time) + "] Passenger #" +
                           std::to_string(p->id()) +
                           " waiting elevator at floor " +
                           std::to_string(p->boarding_floor()));
  }
}

Elevator *ElevatorSystem::calculate_most_suitable_elevator(size_t floor) {
  Elevator *best_idle_elevator = nullptr;
  int min_idle_distance = std::numeric_limits<int>::max();

  Elevator *best_moving_elevator = nullptr;
  int min_moving_distance = std::numeric_limits<int>::max();

  for (auto &elevator : m_elevators) {
    int current_distance = abs(static_cast<int>(elevator.current_floor()) -
                               static_cast<int>(floor));
    ElevatorState elevator_state = elevator.state();
    ElevatorState state = elevator.current_floor() < floor
                              ? ElevatorState::MovingUp
                              : ElevatorState::MovingDown;

    if (elevator_state == ElevatorState::IdleClosed ||
        elevator_state == ElevatorState::IdleOpen) {
      if (current_distance < min_idle_distance) {
        min_idle_distance = current_distance;
        best_idle_elevator = &elevator;
      }
      continue;
    }

    if (elevator_state == state) {
      if (current_distance < min_moving_distance) {
        min_moving_distance = current_distance;
        best_moving_elevator = &elevator;
      }
    }
  }

  if (best_idle_elevator != nullptr) {
    return best_idle_elevator;
  }

  return best_moving_elevator;
}

void ElevatorSystem::interrupt_elevator(Elevator *elevator,
                                        size_t target_floor) const {
  auto &buttons = elevator->pressed_buttons();
  buttons[target_floor] = true;
  elevator->set_target_floor(target_floor);
  ElevatorState state = elevator->current_floor() < target_floor
                            ? ElevatorState::MovingUp
                            : ElevatorState::MovingDown;
  elevator->set_state(state, m_time);
  elevator->calculate_moving_time(m_time);
  information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                         std::to_string(elevator->id()) +
                         " interrupted to floor " +
                         std::to_string(target_floor));
}
