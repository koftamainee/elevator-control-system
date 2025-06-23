#pragma once

#include <cstddef>
#include <set>

class Passenger final {
 private:
  size_t m_id;
  size_t m_appear_time;
  size_t m_boarding_floor;
  size_t m_target_floor;
  double m_weight;
  size_t m_boarding_time = 0;
  size_t m_deboarding_time = 0;

  bool m_has_overload_lift = false;
  std::set<Passenger*> m_met_passengers;

 public:
  Passenger(size_t id, size_t appear_time, size_t boarding_floor,
            size_t target_floor, double weight)
      : m_id(id),
        m_appear_time(appear_time),
        m_boarding_floor(boarding_floor),
        m_target_floor(target_floor),
        m_weight(weight) {}

  size_t id() const noexcept { return m_id; }
  size_t appear_time() const noexcept { return m_appear_time; }
  size_t boarding_floor() const noexcept { return m_boarding_floor; }
  size_t target_floor() const noexcept { return m_target_floor; }
  double weight() const noexcept { return m_weight; }
  bool has_overload_lift() const noexcept { return m_has_overload_lift; }
  void add_met_passenger(Passenger* passenger) {
    m_met_passengers.insert(passenger);
  }
  void set_deboarding_time(size_t time) { m_deboarding_time = time; }

  void set_overload_lift() { m_has_overload_lift = true; }
  size_t boarding_time() const { return m_boarding_time; }
  size_t deboarding_time() const { return m_deboarding_time; }

  bool has_met_passenger(Passenger* const passenger) const {
    return m_met_passengers.contains(passenger);
  }

  const std::set<Passenger*>& met_passengers() const {
    return m_met_passengers;
  }
};
