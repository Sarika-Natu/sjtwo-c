SID:014529138
LAB7_UART

Part0, Part1, Part2 is commented. Part3 is extended.

The lab provides UART communication protocol. 
UART interrupt is implemented.
Queues are implemented to put and get characters sent on UART.

Extra functionality: 
1. Random numbers are generated and sent on UART channel. 
The same numbers are received on UART channel.
2. MultiUART functionality implemented for UART driver.

UART3 is used on port P4.28 - TXD and P4.29 - RXD
UART2 is working on port P2.8 - TXD and P2.9 - RXD

Saleae Logic Analyzer is used to capture waveforms of UART communication.

Screenshots of Telemetry and Logic analyzer for all parts are in LAB7 folder.
	