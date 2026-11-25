#include <config/TestConfigured.hpp>
#include <config/TestShared.hpp>

bool good_implementation();  // Base implementation selection worked
bool good_override();        // Override implementation selection worked
bool good_configured();      // Last global choice of an implementation with CONFIGURATION_HEADERS worked

// The headers placed in config/ are those of the implementations that are linked
static_assert(TEST_CONFIGURED_VALUE == 2, "config/TestConfigured.hpp is not that of Test_Configured_Second");
static_assert(TEST_SHARED_VALUE == 3, "config/TestShared.hpp is not the shared header");

// No operation executable
int main(int argc, char** argv) {
    return static_cast<int>(good_implementation() && good_override() && good_configured());
}
