#include "SerialPort.h"
#include <iostream>

SerialPort::SerialPort(char* portName, DWORD baudRate) {
	errors = 0;
	status = { 0 };
	connected = false;

	handleToCOM = CreateFileA(static_cast<LPCSTR>(portName), GENERIC_READ | GENERIC_WRITE, 
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); // Assign it is a file... idk what half this stuff does

	DWORD errorMsg = GetLastError();

	if (errorMsg == 2) {
		printf("No device found.\n");
		return;
	}
	else if (errorMsg == 5) {
		printf("Access denied, another program is currently using port.\n");
		return;
	}
	else if (errorMsg != 0) {
		// Any other error not caught by other error handlers
		printf("Error.\n");
		return;
	}

	DCB dcbSerialParameters = { 0 };

	if (!GetCommState(handleToCOM, &dcbSerialParameters)) {
		printf("Failed to get serial params.\n");
		return;
	}

	dcbSerialParameters.BaudRate = baudRate;
	dcbSerialParameters.ByteSize = 8;
	dcbSerialParameters.StopBits = ONESTOPBIT;
	dcbSerialParameters.Parity = NOPARITY;
	dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

	if (!SetCommState(handleToCOM, &dcbSerialParameters)) {
		printf("Failed to set serial params.\n");
		return;
	}

	connected = true;
	PurgeComm(handleToCOM, PURGE_RXCLEAR | PURGE_TXCLEAR); // Clear all previously saved serial data
	Sleep(2000);
}

SerialPort::~SerialPort() {
	if (!connected) return;
	connected = false;
	CloseHandle(handleToCOM);
}

bool SerialPort::isAvailable() {
	ClearCommError(handleToCOM, &errors, &status);
	//std::cout << status.cbInQue << std::endl;
	return status.cbInQue > 0;
}

char* SerialPort::read() {
	ClearCommError(handleToCOM, &errors, &status);

	char buffer[1]; // Buffer to assign read byte(s) to
	DWORD bytesRead; // Number representing number of bytes succesfully read

	// ReadFile reads the handleToCOM and assigns the input number of bytes to read in the buffer array
	// In this case, I pass 1 byte to be read, so the char buffer is length 1
	if (ReadFile(handleToCOM, buffer, 1, &bytesRead, NULL)) {
		//printf("Successfully read");
		return buffer;
	}
	else {
		//printf("Failed to read");
		// Return NULL if nothing is read
		std::cout << GetLastError() << std::endl;
		return NULL;
	}
}

bool SerialPort::isConnected() {
	return connected;
}