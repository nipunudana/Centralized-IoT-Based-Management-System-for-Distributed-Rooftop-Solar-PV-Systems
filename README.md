This project aims to develop a centralized IoT-based platform for monitoring, simulating, controlling, and analyzing rooftop solar photovoltaic (PV) systems, along with modeling the low-voltage distribution network to assess solar integration feasibility.

The first part of the system uses ESP32 microcontrollers and current transformers to measure inverter output and system performance. Relays are used to simulate remote ON/OFF control of existing inverters, while RS485 communication (using MAX485 modules) is implemented for new inverters to enable direct data acquisition and control. The collected data is transmitted via MQTT to a cloud platform (ThingsBoard) for centralized visualization and analysis of multiple rooftop systems in real time.

A simulated control feature is also integrated to demonstrate grid management strategies during disturbances or low-demand conditions.

In the second part of the project, the existing low-voltage distribution network is modeled in ETAP to perform power flow analysis, evaluating the network’s capability to support further rooftop solar integration.

Together, this integrated system provides a smart, scalable, and practical solution for managing distributed rooftop solar PV systems—enhancing inverter communication, remote control, and overall grid stability.
