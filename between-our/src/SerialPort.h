#pragma once

#define ARDUINO_WAIT_TIME 2000
#define INPUT_DATA_BYTES 7

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

class SerialPort
{
private:
	HANDLE handleToCOM; // Treat USB port as an io file stream
	bool connected;
	COMSTAT status;
	DWORD errors;
public:
	SerialPort(char* portName, DWORD baudRate);
	~SerialPort(); // Destructor

	bool isAvailable(); // Returns whether at least 1 byte is readable from the port
	char* read(); // Returns the byte if isAvailable() returns true

	bool isConnected();
};