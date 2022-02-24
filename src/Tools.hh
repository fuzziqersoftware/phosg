#pragma once

#include <functional>



class CallOnDestroy {
public:
  CallOnDestroy(std::function<void()> f);
  ~CallOnDestroy();
private:
  std::function<void()> f;
};

inline CallOnDestroy on_close_scope(std::function<void()> f) {
  return CallOnDestroy(move(f));
}
