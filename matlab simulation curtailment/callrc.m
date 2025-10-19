P = 26000;            % Power in Watts
Vin = 11*11;          % Input Voltage
fs = 10e3;            % Switching frequency (Hz)
Vout = 600;           % Output Voltage

Ioutmax = P / Vout;                               % Max output current
delIL = 0.01 * Ioutmax * (Vout / Vin);            % Inductor ripple current
delVout = 0.01 * Vout;                            % Output voltage ripple

L = (Vin * (Vout - Vin)) / (delIL * fs * Vout)  % Inductor value
C = (Ioutmax * (1 - (Vin / Vout))) / (fs * delVout) % Capacitor value
R = Vout / Ioutmax                              % Load resistance
d = (Vout - Vin) / Vout                          % Duty cycle
