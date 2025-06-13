#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "client_logger_builder.h"
#include "elevator.h"
#include "elevator_system.h"
#include "logger.h"

std::pair<std::vector<Elevator>, size_t> parse_elevators_file(
    std::string const &file) {
  std::ifstream fin(file);
  if (!fin.is_open()) {
    throw std::runtime_error("Failed to open configuration file: " + file);
  }

  fin.seekg(0, std::ios::end);
  if (fin.tellg() == 0) {
    throw std::runtime_error("Configuration file is empty: " + file);
  }
  fin.seekg(0, std::ios::beg);

  size_t n_floors = 0;
  size_t k_elevators = 0;

  if (!(fin >> n_floors >> k_elevators)) {
    throw std::runtime_error("Failed to read number of floors and elevators");
  }

  if (n_floors == 0) {
    throw std::runtime_error("Number of floors (n) must be positive");
  }
  if (k_elevators == 0) {
    throw std::runtime_error("Number of elevators (k) must be positive");
  }

  std::vector<double> max_loads;
  max_loads.reserve(k_elevators);

  for (size_t i = 0; i < k_elevators; ++i) {
    double max_load;
    if (!(fin >> max_load)) {
      throw std::runtime_error("Failed to read max_load for elevator " +
                               std::to_string(i + 1) + ". Expected " +
                               std::to_string(k_elevators) + " values");
    }

    if (max_load <= 0) {
      throw std::runtime_error(
          "Invalid max_load for elevator " + std::to_string(i + 1) +
          ": must be positive (got " + std::to_string(max_load) + ")");
    }
    max_loads.push_back(max_load);
  }

  std::string extra_data;
  if (fin >> extra_data) {
    throw std::runtime_error(
        "Unexpected data in configuration file after elevator specifications: "
        "'" +
        extra_data + "'");
  }

  std::vector<Elevator> elevators;
  elevators.reserve(k_elevators);
  for (size_t i = 0; i < k_elevators; ++i) {
    elevators.emplace_back(1, max_loads[i], n_floors);
  }

  return {std::move(elevators), n_floors};
}

int main(int argc, char **argv) {
  if (argc < 5) {
    std::cerr << "Not enougth command line arguments.\nUsage: " << argv[0]
              << " <input_elevators_file> <input_passengers_file> "
                 "<output_passengers_file> <output_elevators_file>"
              << std::endl;
    return 1;
  }

  try {
    std::unique_ptr<logger> log(
        client_logger_builder()
            .add_file_stream("files/runtime.log", logger::severity::information)
            ->add_console_stream(logger::severity::information)
            ->build());
    auto [elevators, floors_count] = parse_elevators_file(argv[1]);
    log->information(
        "Parsed elevators file. Results: " + std::to_string(elevators.size()) +
        " elevators, " + std::to_string(floors_count) + " floors");

    ElevatorSystem(elevators, floors_count, log.get())
        .model(argv[2])
        .print_results(argv[3], argv[4]);
    return 0;
  } catch (std::exception const &e) {
    std::cerr << "Runtime error occured during the execution: " << e.what()
              << std::endl;
    return 1;
  }
}
