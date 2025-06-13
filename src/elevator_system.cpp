#include "elevator_system.h"

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
      m_waiting_passengers_by_floor(floors_count + 1) {}

ElevatorSystem &ElevatorSystem::model(std::string const &input_file) {
  parse_passengers_file(input_file);
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
  elevator->set_state(ElevatorState::IdleOpen);
  // TODO: remove arrived passangers from elevator

  auto &waiting_queue = m_waiting_passengers_by_floor[floor];

  auto it = waiting_queue.begin();

  while (it != waiting_queue.end()) {
    Passenger *next_passenger = *it;
    if (elevator->try_move_passenger_in(next_passenger)) {
      it = waiting_queue.erase(it);
    } else {
      ++it;
    }
  }
  // TODO: move to next floor
}
