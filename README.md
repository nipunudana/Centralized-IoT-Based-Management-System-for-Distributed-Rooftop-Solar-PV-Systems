This project aims to develop a centralized IoT-based platform for monitoring, simulating, controlling, and analyzing rooftop solar photovoltaic (PV) systems, along with modeling the low-voltage distribution network to assess solar integration feasibility.

The first part of the system uses ESP32 microcontrollers and current transformers to measure inverter output and system performance. Relays are used to simulate remote ON/OFF control of existing inverters, while RS485 communication (using MAX485 modules) is implemented for new inverters to enable direct data acquisition and control. The collected data is transmitted via MQTT to a cloud platform (ThingsBoard) for centralized visualization and analysis of multiple rooftop systems in real time.

A simulated control feature is also integrated to demonstrate grid management strategies during disturbances or low-demand conditions.

In the second part of the project, the existing low-voltage distribution network is modeled in ETAP to perform power flow analysis, evaluating the network’s capability to support further rooftop solar integration.

Together, this integrated system provides a smart, scalable, and practical solution for managing distributed rooftop solar PV systems—enhancing inverter communication, remote control, and overall grid stability.


🛰️ Centralized IoT-Based Management System for Distributed Rooftop Solar PV Systems

This project presents a centralized IoT-based platform for real-time monitoring, simulated control, and power system analysis of distributed rooftop solar photovoltaic (PV) systems.

Built using ESP32 microcontrollers, current transformers (CTs), and the ThingsBoard IoT platform, the system enables utilities and operators to visualize inverter outputs, track solar generation, and remotely control inverter operation through MQTT-based communication.

In addition, an ETAP-based network simulation is used to model the low-voltage (LV) distribution system, allowing analysis of solar integration feasibility, voltage stability, and transformer loading under varying operating conditions.

🚀 Key Features

⚡ Real-time Monitoring – Measures inverter output current and power via IoT-connected ESP32 nodes.

☁️ Cloud-Based Dashboard – Centralized visualization and control using ThingsBoard with hierarchical navigation.

🔄 Relay ON/OFF Simulation – Initial control achieved using a relay-based inverter switching mechanism.

🔌 Direct Inverter Communication (RS485/Modbus) – Later upgraded to enable direct inverter data acquisition and control via MAX485 and Modbus RTU protocol.

🧠 Smart Grid Integration – Combines live IoT data with ETAP simulations for better operational decision-making.

🗺️ Hierarchical Dashboard View:

Transformer Level → overview of connected transformers with live loading and voltage data

Customer Level → lists customers under each transformer with aggregated solar generation

Inverter Level → real-time inverter data and ON/OFF control interface

🧰 Technologies Used
| Category                    | Tools / Components                                               |
| --------------------------- | ---------------------------------------------------------------- |
| **Hardware**                | ESP32, SCT-013 Current Transformer, Relay Module, RS485 (MAX485) |
| **Software**                | Arduino IDE, ThingsBoard (MQTT), ETAP, LTspice, EmonLib          |
| **Communication Protocols** | MQTT, Modbus RTU (via RS485)                                     |
| **Languages**               | C++, Python, SQL (for ThingsBoard backend)                       |


📊 System Highlights

Real-time IoT dashboard powered by ThingsBoard with multi-level monitoring (Transformer → Customer → Inverter)

MQTT-based data flow for fast and reliable updates

Remote firmware management and OTA updates via GitHub integration

ETAP model for evaluating feeder capacity, voltage profiles, and reverse power flow risks

Modular system design for scaling across multiple rooftop PV sites

📈 Outcomes

This project demonstrates a scalable smart grid framework that integrates IoT monitoring with power network simulation tools.
The final prototype successfully:

Collected inverter current and power data in real time

Supported both relay-based and direct RS485/Modbus-based inverter control

Enabled cloud-based visualization and command control via ThingsBoard

Validated network performance and solar hosting capacity using ETAP modeling

Together, these components provide a strong foundation for future large-scale distributed solar management and smart energy systems.

