#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "game-engine/types.h"
#include "game-engine/WorldObject.h";
#include "game-engine/World.h"
#include "game-engine/Camera.h"
#include "game-engine/Player.h"
#include "game-engine/ui/uiHeader.h"
#include <string>
#include <future> 
#include <mutex> 
#include <chrono> 
#include "SerialPort.h"

#define PACKET_LENGTH 22
#define STEP 0.1

using namespace gameEngine::ui;
using namespace gameEngine;

char comPort[] = "COM5"; // Change this if you want to use a different USB port
char* port = comPort;
SerialPort mySerial(port, CBR_115200); // Declare serial port

TextInput nameInput(Pointf(-1920 / 4 + 30, 1080 / 4 - 50), "Enter name");
const char* nameInputVal = " ";
Button goButton(Pointf(-50, 0), Dimensionf(100, 50));
Text goText(Pointf(10, 10), "Start");

Screen initialScreen;

Camera camera;
int following = 1;

World world(500, 500);

Player p1("");
#define NUM_POINTS 360
WorldObject points[NUM_POINTS];
ColorSquare pointColors[NUM_POINTS];

int distances[NUM_POINTS];

GameScreen screen(&world, &camera);

Text canKill(Pointf(-1920/4 + 30, 1080/4 - 40), "Kill (f)");

#define NUM_THREADS 2
std::future<void> threads[NUM_THREADS];

PolarPointf toPolarPointf(float x, float y) {
	float r = sqrt(x * x + y * y);
	float theta = atan(y / x);
	if (x < 0) theta += 180;
	return PolarPointf(r, theta);
}

PolarPointf toPolarPointf(Pointf p) {
	return toPolarPointf(p.x, p.y);
}

Pointf toPointf(float r, float theta) {
	float x = r * cos(theta * 3.14 / 180);
	float y = r * sin(theta * 3.14 / 180);
	return Pointf(x, y);
}

Pointf toPointf(PolarPointf p) {
	return toPointf(p.r, p.theta);
}

// Unused function to remove wall data
void processFrame(int distances[NUM_POINTS]) {
	std::cout << "Processing frame" << std::endl;
	Pointf p[NUM_POINTS];
	int tempIndex = 0;
	float dydx = 0.0;
	for (int i = 0; i < NUM_POINTS; i++) {
		std::cout << i << std::endl;
		PolarPointf polar(distances[i], i);
		p[i] = toPointf(polar);

		if (i == 0) continue;

		float temp = (p[i - 1].y - p[i].y) / (p[i - 1].x - p[i].x);

		if (abs(dydx - temp) > 5) {
			if (i - tempIndex > 3) {
				for (int j = tempIndex; j < i; j++) {
					p[i] = Pointf(0, 0);
				}
			}
			tempIndex = i;
		}

		dydx = temp;

		points[i].setPosition(p[i]);
	}
}

// Read a single packet and update point data
void readSerial() {
	if (!mySerial.isConnected() || !mySerial.isAvailable()) return; // Instantly quit if serial isn't available or connected

	char* buffer = mySerial.read(); // Read serial data, check SerialPort.cpp for how that works

	if (buffer == nullptr) return; // Check if error happened while reading data

	unsigned char b = buffer[0];
	// Start byte is always hex FA (which can be 0xfa in C++) which is equivalent to 1111 1100 in binary
	// Return if the first byte read is not the start byte
	if (b != 0xfa) return;

	// Continue if start byte is correct

	unsigned char data[PACKET_LENGTH];
	int i = 1;
	data[0] = b;

	// Loop through and read serial until the whole packet is read
	while (i < PACKET_LENGTH) {
		if (!mySerial.isAvailable()) continue; // Wait until a byte is available to read
		char* temp = mySerial.read(); // Read serial, more in SerialPort.cpp
		if (temp == nullptr) continue; // If no data is return, retry
		// Assign read byte into our data array (add 255 to because unsigned -> signed data conversion can be funky)
		data[i] = temp[0] + 256;
		i++;
	}

	int index = int(data[1]) - 0xa0; // Index represents the angle of the data / 4
	// (So if index = 0, angles would be 0 deg to 3 deg, index 89 would be angles 356 deg to 359 deg)

	/*if (index == 0) {
		threads[2] = std::async(std::launch::async, processFrame, distances);
	}*/

	for (int j = 0; j < 4; j++) {
		int angle = index * 4 + j;
		int lowDist = data[4 + 4 * j];
		int highDist = data[5 + 4 * j];
		int lowQuality = data[6 + 4 * j];
		int highQuality = data[7 + 4 * j];

		if (highDist & 0xe0) {
			points[angle].setPosition(0, 0);
			std::cout << "Error!" << std::endl;
			continue;
		}

		int dist = highDist & 0x1f;
		dist <<= 8;
		dist |= lowDist;
		//dist /= 20;

		int quality = highQuality;
		quality <<= 8;
		quality |= lowQuality;
		quality /= 2;

		float r, g;

		if (quality < 256) {
			g = quality;
			r = 256;
		}
		else {
			r = 256 - (quality - 256);
			g = 256;
		}

		r /= 256.0;
		g /= 256.0;
		
		std::cout << "angle: " << angle << ", distance: " << dist << ", quality: " << quality * 128 << std::endl;
		//distances[angle] = dist;

		PolarPointf polar(dist, angle);
		Pointf p = toPointf(polar);
		points[angle].setPosition(p);
		
		/*ColorSquare c(Color3f(r, g, 0));
		points[angle].setTexture(&c);*/
		pointColors[angle].setColor(Color3f(r, g, 0));
	}
}

void loopSerial() {
	while (true) {
		readSerial();
	}
}

int main() {
	ViewStyle btnStyle;
	btnStyle.bgColor = Color3f(0, 0, 1);
	goButton.setStyle(btnStyle);
	goButton.addUiElement(&goText);
	goButton.setOnPress([](int button, bool action) -> void {
		p1.setName(nameInputVal);
		window::navigate(&screen);

		window::setRenderCallback([](double delta) -> void {
			//readSerial();
			world.gameTick(delta);
			camera.gameTick(delta);

			

			/*if (p1.isInRange(&p2) && !p2.isDead()) {
				canKill.setVisible(true);
			}
			else {
				canKill.setVisible(false);
			}*/
		});
	});

	/*nameInput.setOnChangeText([](char* text) -> void {
		nameInputVal = text;
		});*/

	initialScreen.addUiElement(&goButton);
	//initialScreen.addUiElement(&nameInput);

	//p2.setPlayerColor(BLUE);

	world.addWorldObject(&p1);
	//world.addWorldObject(&p2);

	for (int i = 0; i < NUM_POINTS; i++) {
		PolarPointf polar(0, i);
		Pointf p = toPointf(polar);
		points[i].setPosition(p);
		points[i].setSize(30, 30);
		pointColors[i].setColor(Color3f(1, 1, 1));
		points[i].setTexture(&pointColors[i]);
		world.addWorldObject(&(points[i]));
	}

	camera.follow(&p1);

	screen.setKeyPress(GLFW_KEY_W, [](bool action) -> void {
		p1.setMovingUp(action);
	});
	screen.setKeyPress(GLFW_KEY_A, [](bool action) -> void {
		p1.setMovingLeft(action);
	});
	screen.setKeyPress(GLFW_KEY_S, [](bool action) -> void {
		p1.setMovingDown(action);
	});
	screen.setKeyPress(GLFW_KEY_D, [](bool action) -> void {
		p1.setMovingRight(action);
	});

	screen.setKeyPress(GLFW_KEY_UP, [](bool action) -> void {
		camera.setZoomingIn(action);
		// p2.setMovingUp(action);
		});
	screen.setKeyPress(GLFW_KEY_LEFT, [](bool action) -> void {
		// p2.setMovingLeft(action);
		});
	screen.setKeyPress(GLFW_KEY_DOWN, [](bool action) -> void {
		camera.setZoomingOut(action);
		// p2.setMovingDown(action);
		});
	screen.setKeyPress(GLFW_KEY_RIGHT, [](bool action) -> void {
		// p2.setMovingRight(action);
		});

	screen.setKeyPress(GLFW_KEY_SPACE, [](bool action) -> void {
		/*if (!action) return;

		if (following == 1) {
			following = 2;
			camera.follow(&p2);
		}
		else {
			following = 1;
			camera.follow(&p1);
		}*/
		});

	screen.setKeyPress(GLFW_KEY_F, [](bool action) -> void {
		//if (p1.isInRange(&p2)) p1.attemptToKill(&p2);
		});


	window::navigate(&initialScreen);

	//window::startWindow(1920/2, 1080/2);

	threads[0] = std::async(std::launch::async, window::startWindow, 1920 / 2, 1080 / 2);
	threads[1] = std::async(std::launch::async, loopSerial);

	return 0;
}