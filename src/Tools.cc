#include "Tools.hh"

namespace phosg {

CallOnDestroy::CallOnDestroy(std::function<void()> f) : f(f) {}

CallOnDestroy::~CallOnDestroy() {
  this->f();
}

} // namespace phosg
