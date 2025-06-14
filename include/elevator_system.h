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
  std::vector<bool> m_pending_lift_calls;
  int m_remaining_passengers = 0;
  int test_passengers_appeared_on_starting_floors = 0;
  int test_pasengers_succesfully_moved_to_dest = 0;

  std::multimap<size_t, Passenger *> m_time_index;
  std::set<size_t> m_floors_already_called_elevator;

  size_t m_time = 0;

  logger *log = nullptr;

  logger *get_logger() const override { return log; }

  void parse_passengers_file(std::string const &file);

  size_t time_to_numerical(std::string const &time) const;

  void process_floor_arival(size_t floor, Elevator *elevator);
  void process_passengers_deboarding(size_t floor, Elevator *elevator);
  void move_passengers_from_floor_to_elevator(size_t floor, Elevator *elevator);
  void calculate_next_elevator_target(size_t floor, Elevator *elevator) const;
  void arrive_passengers(size_t current_time);
  Elevator *calculate_most_suitable_elevator(size_t floor);
  void interrupt_elevator(Elevator *elevator, size_t target_floor) const;

 public:
  ElevatorSystem(std::vector<Elevator> elevators, size_t floors_count,
                 logger *log);
  ElevatorSystem &model(std::string const &input_file);
  ElevatorSystem &print_results(std::string const &passengers_file_path,
                                std::string const &elevators_file_path);
};
