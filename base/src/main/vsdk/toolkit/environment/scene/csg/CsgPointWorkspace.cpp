#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWorkspace.h"
CsgPointWorkspace::CsgPointWorkspace() : path(8), results(16), events(16) {}
CsgPointWorkspace::~CsgPointWorkspace() {}
void CsgPointWorkspace::clear() { path.clear(); results.clear(); events.clear(); }
