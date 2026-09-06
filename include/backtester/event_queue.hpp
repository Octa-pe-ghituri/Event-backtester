#pragma once

#include <queue>
#include <vector>

template <typename Event> class EventQueue {

private:
  struct EventInQueue {
    Event event;
    long long order;
  };

  struct Compare_Events {

    bool operator()(const EventInQueue &lhs, const EventInQueue &rhs) const {

      if (lhs.event.time == rhs.event.time) {
        return lhs.order > rhs.order;
      }

      return lhs.event.time > rhs.event.time;
    };
  };

  std ::priority_queue<EventInQueue, std::vector<EventInQueue>, Compare_Events>
      events;
  long long order_counter = 0;

public:
  void AddEvent(const Event &event) {

    events.push({event, order_counter});
    order_counter++;
  }

  bool IsEmpty() const { return events.empty(); }

  std ::vector<Event> GetEvents(const int &time) {

    std::vector<Event> result;

    while (!events.empty() && events.top().event.time <= time) {

      result.push_back(events.top().event);
      events.pop();
    }

    return result;
  }
};
