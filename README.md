# SmartTraffix - Traffic Management System

A real-time traffic management simulation system that models vehicle movement, traffic signals, and automated challan (traffic ticket) generation. The system includes features for traffic violation detection, payment processing, and traffic statistics monitoring.
![Image](https://github.com/user-attachments/assets/2c96ce64-db1c-4d63-98ec-734ff7e02390)
## Features

- Multi-threaded traffic simulation
- Realistic vehicle behavior with different types:
  - Light vehicles (cars)
  - Heavy vehicles (trucks)
  - Emergency vehicles (ambulances)
- Automated traffic signal management
- Speed violation detection
- Real-time challan generation
- Payment processing system
- Traffic statistics monitoring
- Mock time system for testing different traffic scenarios

## Components

### Core Simulation
- `main.cpp` - Entry point of the simulation
- `headers/simulation.h` - Main simulation controller
- `headers/vehicle.h` - Vehicle class implementation
- `headers/vehiclespawner.h` - Vehicle generation and management
- `headers/trafficmanager.h` - Traffic signal and violation management
- `headers/util.h` - Utility functions and constants

### Challan System
- `challan.cpp` - Traffic violation ticket generation and management
- `stripepayment.cpp` - Payment processing interface
- `userportal.cpp`

## Compilation

The project includes a compilation script `compile_run.sh`. To compile and run:

1. Make sure SFML is installed on your system
2. Run the compilation script:

```bash
chmod +x compile_run.sh
./compile_run.sh
```

## Usage

After successful compilation, the system will launch multiple windows:
- Main traffic simulation window
- Traffic statistics window
- Challan management window
- User portal window
- Payment processing window (opens when needed)

The simulation runs for 5 minutes by default (configurable in util.h).

## Traffic Rules

- Light vehicles speed limit: 60 km/h
- Heavy vehicles speed limit: 40 km/h
- Emergency vehicles speed limit: 80 km/h
- Heavy vehicles are restricted during peak hours:
  - Morning: 7:00 AM - 9:30 AM
  - Evening: 4:30 PM - 8:30 PM

## Challan System

Violations that trigger automatic challans:
- Light vehicles exceeding 60 km/h
- Heavy vehicles exceeding 40 km/h
- Fine amounts:
  - Light vehicles: 5000 PKR + 17% service charge
  - Heavy vehicles: 7000 PKR + 17% service charge

