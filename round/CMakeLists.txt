# Copyright (c) 2015 Hartmut Kaiser
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(category round)
set(programs round)

foreach(program ${programs})

  set(${program}_FLAGS DEPENDENCIES iostreams_component)

  set(sources ${program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add executable
  add_hpx_executable(${program}
                     SOURCES ${sources}
                     ${${program}_FLAGS}
                     FOLDER "Benchmarks")

  add_hpx_pseudo_target(benchmarks.${category}.${program})
  add_hpx_pseudo_dependencies(benchmarks.${category} benchmarks.${category}.${program})
  add_hpx_pseudo_dependencies(benchmarks.${category}.${program} ${program}_exe)

endforeach()

