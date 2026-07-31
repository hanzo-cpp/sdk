# The six canonical flows — the same journeys in every Hanzo SDK, named and
# ordered by hanzoai/openapi's flows.yaml. An example that does not compile is
# worse than no example, so CI builds all six; what they print is this
# language's business, which routes they call is not.
#
# hanzo_examples is the ONE place the environment resolves — key, base URL, org
# — so no flow reads getenv itself.
add_library(hanzo_examples STATIC hanzo.cpp)
target_include_directories(hanzo_examples PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hanzo_examples PUBLIC hanzo::hanzo)

foreach(flow hello chat money store agent tools)
  add_executable(${flow} ${flow}/main.cpp)
  target_link_libraries(${flow} PRIVATE hanzo_examples)
endforeach()
