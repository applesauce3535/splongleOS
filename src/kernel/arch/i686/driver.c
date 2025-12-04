#include "include/driver.h"

void AddDriver(Driver* driver) {
    g_Manager.drivers[g_Manager.num_drivers] = driver;
    g_Manager.num_drivers++;
}