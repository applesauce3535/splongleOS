#pragma once

typedef struct {
    void (*Activate)(void);
    int (*Reset)(void);
    void (*Deactivate)(void);
} Driver;

typedef struct {
    Driver* drivers[255];
    int num_drivers;
    void (*AddDriver)(Driver* driver);
} DriverManager;

void AddDriver (Driver* driver);

// this is the driver manager for the entire system
static DriverManager g_Manager = {
    .num_drivers = 0,
    .AddDriver = AddDriver
};