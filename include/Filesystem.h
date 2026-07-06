#pragma once

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#include <experimental/filesystem>
namespace stockflow { namespace fs = std::experimental::filesystem; }
#else
#include <filesystem>
namespace stockflow { namespace fs = std::filesystem; }
#endif
