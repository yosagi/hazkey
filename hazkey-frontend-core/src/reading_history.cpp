#include "hazkey/frontend/reading_history.h"

#include <algorithm>

namespace hazkey::frontend {

void ReadingHistory::setCapacity(int capacity) {
    capacity_ = static_cast<size_t>(std::max(capacity, 0));
    trim();
}

void ReadingHistory::push(const std::string& reading) {
    if (reading.empty() || capacity_ == 0) return;
    auto it = std::find(readings_.begin(), readings_.end(), reading);
    if (it != readings_.end()) {
        readings_.erase(it);
    }
    readings_.push_front(reading);
    trim();
}

void ReadingHistory::trim() {
    while (readings_.size() > capacity_) {
        readings_.pop_back();
    }
}

}  // namespace hazkey::frontend
