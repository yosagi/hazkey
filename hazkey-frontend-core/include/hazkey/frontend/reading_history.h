#ifndef HAZKEY_FRONTEND_READING_HISTORY_H_
#define HAZKEY_FRONTEND_READING_HISTORY_H_

#include <cstddef>
#include <deque>
#include <string>

namespace hazkey::frontend {

// Readings of recent compositions, newest first, kept so that a composition
// lost to the application (committed or dropped on a click or focus out)
// can be brought back without typing it again. One instance per client
// process, shared by its state machines. Kept in memory only.
class ReadingHistory {
   public:
    // 0 disables the history and forgets what is kept
    void setCapacity(int capacity);

    // Add a reading as the newest. The same reading kept already moves to
    // the front instead of being added twice. Empty readings are ignored.
    void push(const std::string& reading);

    size_t size() const { return readings_.size(); }
    bool empty() const { return readings_.empty(); }
    // 0 is the newest
    const std::string& at(size_t index) const { return readings_[index]; }

   private:
    void trim();

    size_t capacity_ = 5;
    std::deque<std::string> readings_;
};

}  // namespace hazkey::frontend

#endif  // HAZKEY_FRONTEND_READING_HISTORY_H_
