#include "elevator_system.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
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
  information_with_guard(
      "Modeling "
      "starts!\n-----------------------------------------------------------");

  while (m_remaining_passengers > 0) {
    // getchar();
    arrive_passengers(m_time);
    for (int i = 1; i < m_waiting_passengers_by_floor.size(); ++i) {
      auto &pas_list = m_waiting_passengers_by_floor.at(i);
      if (pas_list.size() > 0 &&
          !m_floors_already_called_elevator.contains(i)) {
        Elevator *e = calculate_most_suitable_elevator(i);
        if (e != nullptr) {
          m_floors_already_called_elevator.insert(i);
          if (e->current_floor() == i &&
              (e->state() == ElevatorState::IdleClosed)) {
            process_floor_arival(i, e);
          } else {
            interrupt_elevator(e, i);
          }
        }
      }
    }
    for (auto &e : m_elevators) {
      if (m_time >= e.time_travel_ends() && e.target_floor() > 0) {
        process_floor_arival(e.target_floor(), &e);
      }
    }

    ++m_time;
  }

  for (auto &e : m_elevators) {
    e.set_state(ElevatorState::IdleClosed, m_time);
  }

  return *this;
}

ElevatorSystem &ElevatorSystem::print_results(
    std::string const &passengers_file_path,
    std::string const &elevators_file_path) {
  std::ofstream passengers_file(passengers_file_path);
  if (passengers_file.is_open()) {
    for (auto const &[id, passenger] : m_passengers) {
      passengers_file << "Passenger " << passenger.id() << ":\n";
      passengers_file << "  Appearance time: " << passenger.appear_time()
                      << "\n";
      passengers_file << "  Origin floor: " << passenger.boarding_floor()
                      << "\n";
      passengers_file << "  Target floor: " << passenger.target_floor() << "\n";
      passengers_file << "  Boarding time: " << passenger.boarding_time()
                      << "\n";
      passengers_file << "  Total travel time: "
                      << (passenger.deboarding_time() -
                          passenger.boarding_time())
                      << "\n";

      passengers_file << "  Met passengers: ";
      bool first = true;
      for (auto const *met_passenger : passenger.met_passengers()) {
        if (!first) {
          passengers_file << ", ";
        }
        passengers_file << met_passenger->id();
        first = false;
      }
      passengers_file << "\n";

      passengers_file << "  Had overload: "
                      << (passenger.has_overload_lift() ? "yes" : "no")
                      << "\n\n";
    }
    passengers_file.close();
  }

  std::ofstream elevators_file(elevators_file_path);
  if (elevators_file.is_open()) {
    for (auto const &elevator : m_elevators) {
      elevators_file << "Elevator " << elevator.id() << ":\n";
      elevators_file << "  Idle time: " << elevator.idle_time() << "\n";
      elevators_file << "  Moving time: " << m_time - elevator.idle_time()
                     << "\n";
      elevators_file << "  Floors passed: " << elevator.floors_passed() << "\n";
      elevators_file << "  Total cargo: " << elevator.total_cargo() << "\n";
      elevators_file << "  Max load reached: " << elevator.max_load_reached()
                     << "\n";
      elevators_file << "  Overloads count: " << elevator.overloads_count()
                     << "\n\n";
    }
    elevators_file.close();
  }

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
  if (elevator == nullptr) {
    throw std::invalid_argument("Null elevator pointer (process_floor_arival)");
  }
  if (floor >= m_waiting_passengers_by_floor.size()) {
    throw std::out_of_range("Invalid floor number");
  }

  elevator->set_floors_passed(
      elevator->floors_passed() +
      std::abs(static_cast<int>(elevator->target_floor() -
                                elevator->current_floor())));

  elevator->set_state(ElevatorState::IdleOpen, m_time);
  elevator->pressed_buttons().at(floor) = false;

  elevator->set_current_floor(floor);
  elevator->set_target_floor(floor);

  int floors_moved = abs(static_cast<int>(floor) -
                         static_cast<int>(elevator->current_floor()));
  elevator->set_floors_passed(elevator->floors_passed() + floors_moved);

  if (m_floors_already_called_elevator.contains(floor)) {
    m_floors_already_called_elevator.erase(floor);
  }
  information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                         std::to_string(elevator->id()) + " arrived at floor " +
                         std::to_string(floor));

  process_passengers_deboarding(floor, elevator);

  move_passengers_from_floor_to_elevator(floor, elevator);
  elevator->set_state(ElevatorState::IdleClosed, m_time);

  calculate_next_elevator_target(floor, elevator);
}

void ElevatorSystem::process_passengers_deboarding(size_t floor,
                                                   Elevator *elevator) {
  if (elevator == nullptr) {
    throw std::runtime_error("nullptr passenenger deboarding");
  }
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
  if (elevator == nullptr) {
    throw std::runtime_error("nullptr move_passengers_from_floor_to_elevator");
  }
  auto &waiting_queue = m_waiting_passengers_by_floor.at(floor);

  auto it = waiting_queue.begin();

  while (it != waiting_queue.end()) {
    Passenger *next_passenger = *it;
    if (elevator->try_move_passenger_in(next_passenger)) {
      it = waiting_queue.erase(it);
      elevator->pressed_buttons().at(next_passenger->target_floor()) = true;
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
  if (elevator == nullptr) {
    throw std::runtime_error("nullptr calculate_next_elevator_target");
  }
  const auto &buttons = elevator->pressed_buttons();

  if (elevator->state() == ElevatorState::MovingUp ||
      elevator->state() == ElevatorState::IdleClosed) {
    for (size_t f = floor + 1; f < buttons.size(); ++f) {
      if (buttons.at(f)) {
        elevator->set_state(ElevatorState::MovingUp, m_time);
        elevator->set_target_floor(f);
        elevator->calculate_moving_time(m_time);
        information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                               std::to_string(elevator->id()) +
                               " continues MovingUp - next target floor " +
                               std::to_string(f) + ", will arrive at [" +
                               std::to_string(elevator->time_travel_ends()) +
                               "]");

        return;
      }
    }

    for (size_t f = floor - 1; f != static_cast<size_t>(-1); --f) {
      if (buttons.at(f)) {
        elevator->set_state(ElevatorState::MovingDown, m_time);
        elevator->set_target_floor(f);
        elevator->calculate_moving_time(m_time);
        information_with_guard(
            "[" + std::to_string(m_time) + "] Elevator #" +
            std::to_string(elevator->id()) +
            " changes direction to MovingDown - next target floor " +
            std::to_string(f) + ", will arrive at [" +
            std::to_string(elevator->time_travel_ends()) + "]");

        return;
      }
    }

  } else if (elevator->state() == ElevatorState::MovingDown ||
             elevator->state() == ElevatorState::IdleClosed) {
    for (size_t f = floor - 1; f != static_cast<size_t>(-1); --f) {
      if (buttons.at(f)) {
        elevator->set_state(ElevatorState::MovingDown, m_time);
        elevator->set_target_floor(f);
        elevator->calculate_moving_time(m_time);
        information_with_guard(
            "[" + std::to_string(m_time) + "] Elevator #" +
            std::to_string(elevator->id()) +
            " continues MovingDown - next target floor " + std::to_string(f) +
            ", will arrive at [" +
            std::to_string(m_time + elevator->time_travel_ends()) + "]");

        return;
      }
    }

    for (size_t f = floor + 1; f < buttons.size(); ++f) {
      if (buttons.at(f)) {
        elevator->set_state(ElevatorState::MovingUp, m_time);
        elevator->set_target_floor(f);
        elevator->calculate_moving_time(m_time);
        information_with_guard(
            "[" + std::to_string(m_time) + "] Elevator #" +
            std::to_string(elevator->id()) +
            " changes direction to MovingUp - next target floor " +
            std::to_string(f) + ", will arrive at [" +
            std::to_string(m_time + elevator->time_travel_ends()) + "]");

        return;
      }
    }
  }

  information_with_guard("[" + std::to_string(m_time) + "] Elevator #" +
                         std::to_string(elevator->id()) +
                         " started idleing (no buttons pressed)");
  elevator->set_state(ElevatorState::IdleClosed, m_time);
  // elevator->set_target_floor(0);
}
void ElevatorSystem::arrive_passengers(size_t current_time) {
  auto range_in_time = m_time_index.equal_range(current_time);
  for (auto it = range_in_time.first; it != range_in_time.second; ++it) {
    Passenger *p = it->second;
    m_waiting_passengers_by_floor.at(p->boarding_floor()).push_back(p);
    test_passengers_appeared_on_starting_floors++;
    information_with_guard(
        "[" + std::to_string(m_time) + "] Passenger #" +
        std::to_string(p->id()) + " waiting elevator at floor " +
        std::to_string(p->boarding_floor()) +
        ", Target floor: " + std::to_string(p->target_floor()));
  }
}

Elevator *ElevatorSystem::calculate_most_suitable_elevator(size_t floor) {
  Elevator *best_elevator = nullptr;
  int min_distance = std::numeric_limits<int>::max();

  std::cout << "finding elevator for interrupt to floor " << floor << std::endl;

  for (auto &elevator : m_elevators) {
    auto current_floor =
        static_cast<int>(elevator.elevator_aproximate_floor(m_time));
    int distance = std::abs(current_floor - static_cast<int>(floor));
    std::cout << "finding elevator. id: " << elevator.id()
              << ", current_floor: " << current_floor
              << ", distance: " << distance;
    ElevatorState elevator_state = elevator.state();

    bool suitable = false;

    if (elevator_state == ElevatorState::IdleClosed ||
        elevator_state == ElevatorState::IdleOpen) {
      suitable = true;
    } else if (elevator_state == ElevatorState::MovingUp) {
      suitable = (current_floor <= static_cast<int>(floor));
    } else if (elevator_state == ElevatorState::MovingDown) {
      suitable = (current_floor >= static_cast<int>(floor));
    }
    std::cout << ". Suitable? " << (suitable ? "true" : "false") << std::endl;

    if (suitable && distance <= min_distance) {
      if (elevator_state == ElevatorState::IdleClosed) {
        if (best_elevator != nullptr) {
          if (best_elevator->state() != ElevatorState::IdleClosed) {
            min_distance = distance;
            best_elevator = &elevator;
          }
        } else {
          min_distance = distance;
          best_elevator = &elevator;
        }
      } else {
        min_distance = distance;
        best_elevator = &elevator;
      }
    }
  }

  if (best_elevator == nullptr) {
    information_with_guard("No suitable elevator found for interrupt");
    //   for (auto &elevator : m_elevators) {
    //     auto current_floor =
    //         static_cast<int>(elevator.elevator_aproximate_floor(m_time));
    //     int distance = abs(current_floor - static_cast<int>(floor));
    //     if (distance < min_distance) {
    //       min_distance = distance;
    //       best_elevator = &elevator;
    //     }
    //   }
  }
  //
  // std::cout << "most suitable elevator id: " << best_elevator->id()
  //           << std::endl;

  return best_elevator;
}

void ElevatorSystem::interrupt_elevator(Elevator *elevator,
                                        size_t target_floor) const {
  if (elevator == nullptr) {
    throw std::runtime_error("nullptr interrupt_elevator");
  }
  if (target_floor == 0) {
    throw std::runtime_error("target floor can not be 0");
  };

  auto &buttons = elevator->pressed_buttons();
  buttons.at(target_floor) = true;

  size_t current_approx_floor = elevator->elevator_aproximate_floor(m_time) + 1;
  elevator->set_current_floor(current_approx_floor);
  calculate_next_elevator_target(current_approx_floor, elevator);

  // ElevatorState
  // new_state = current_approx_floor < target_floor
  //                               ? ElevatorState::MovingUp
  //                               : ElevatorState::MovingDown;
  //
  // elevator->set_target_floor(target_floor);
  // elevator->set_state(new_state, m_time);
  // elevator->calculate_moving_time_with_interrupt(m_time, target_floor);
  //
  // information_with_guard(
  //     "[" + std::to_string(m_time) + "] Elevator #" +
  //     std::to_string(elevator->id()) + " interrupted to floor " +
  //     std::to_string(target_floor) + " (approximate current floor: " +
  //     std::to_string(current_approx_floor) + "), will arrive at [" +
  //     std::to_string(elevator->time_travel_ends()) + "]");
}
