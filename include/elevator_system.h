#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>

#include "elevator.h"
#include "logger_guardant.h"
#include "passenger.h"

class ElevatorSystem final : private logger_guardant {
 private:
  std::vector<Elevator> m_elevators;  // Owner of elevators, elevators borrow
                                      // pointers to passengers to track info
  size_t const m_floors_count;
  size_t const m_elevators_count;
  std::map<size_t, Passenger> m_passengers;  // Owner of passengers
  std::vector<std::list<Passenger *>> m_waiting_passengers_by_floor;

  std::multimap<size_t, Passenger *> m_time_index;

  size_t m_time = 0;

  logger *log = nullptr;

  logger *get_logger() const override { return log; }

  void parse_passengers_file(std::string const &file);

  size_t time_to_numerical(std::string const &time) const;

  void process_floor_arival(size_t floor, Elevator *elevator);

 public:
  ElevatorSystem(std::vector<Elevator> elevators, size_t floors_count,
                 logger *log);
  ElevatorSystem &model(std::string const &input_file);
  ElevatorSystem &print_results(std::string const &passengers_file_path,
                                std::string const &elevators_file_path);
};
