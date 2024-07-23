#include "Tools.hh"

using namespace std;

namespace phosg {

CallOnDestroy::CallOnDestroy(function<void()> f) : f(f) {}

CallOnDestroy::~CallOnDestroy() {
  this->f();
}

} // namespace phosg
