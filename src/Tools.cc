#include "Tools.hh"

using namespace std;

CallOnDestroy::CallOnDestroy(function<void()> f) : f(f) {}

CallOnDestroy::~CallOnDestroy() {
  this->f();
}
