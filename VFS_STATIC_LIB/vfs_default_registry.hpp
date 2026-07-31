#pragma once
#include <any>
#include <string>
#include <vector>
#include <span>
#include <cstddef>
#include <cstring>
#include "vfs_type_registry.hpp"

namespace vfs {

	void register_default_types();

} // namespace vfs

// register_default_types() registers a small set of primitive type
// serializers/deserializers with TypeRegistry. This provides out-of-
// the-box support for storing ints, floats, strings and bools in
// DictionaryFile instances used by examples and tests.

