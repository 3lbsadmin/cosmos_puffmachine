//The setup function is called once at startup of the sketch

#include <EventManager.h>
#include <Button.h>
#include <SimpleTimer.h>

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include "Adafruit_MAX31855.h"

class PumpController
{

	int motorPinMode = 9;
	int speedControlPinMode = 5;
	int currentFrequency = 0;
	int speedValue = 0;

	bool running = false;

public:
	PumpController()
	{
		pinMode(motorPinMode, OUTPUT);
	}

	void Update()
	{
		if (running)
		{
			speedValue = analogRead(speedControlPinMode) / 4;

			if (speedValue != currentFrequency)
			{
				currentFrequency = speedValue;
				analogWrite(motorPinMode, currentFrequency);
			}
		}
	}

	void Start()
	{
		running = true;
	}

	void Stop()
	{
		Reset();
		analogWrite(motorPinMode, 0);
	}

	void Reset()
	{
		running = false;
		currentFrequency = 0;
		speedValue = 0;
		//pinMode( speedControlPinMode, INPUT );
		//analogWrite( pinMode, 124 );
	}
};

class PresureSensor
{
	int sensePinA = 0; // 2 on sensor
	int sensePinB = 1; // 4 on sensor

	int deltaPinAB;
	float pressdiff;

public:
	PresureSensor()
	{
		// analogWrite(v4,255);
		// analogWrite(v3,0);
	}

	void Initialize()
	{

	}

	void Update()
	{
		deltaPinAB = analogRead(sensePinA) - analogRead(sensePinB);
		pressdiff = deltaPinAB * 4.33 / 70; //sensor is 70mV per psi, 1 byte is 4.33mV

		//Serial.print("The Pressure Difference is ");
		//	Serial.println(pressdiff);
	}
};

class PenLightSensor
{
public:
	PenLightSensor()
	{

	}

	void Initialize()
	{

	}

	void Update()
	{

	}
};

class PenTest
{

	bool testStarted = false;

	int sessions = 0;

	PenLightSensor lightController;

	PresureSensor presureController;

	PumpController pumpController;

	const int chipSelect = 10;

	RTC_DS1307 RTC;

	// the logging file
	File logfile;

public:

	PenTest()
	{

		PenLightSensor lightController();

		pinMode(10, OUTPUT);

		// see if the card is present and can be initialized:
		if (!SD.begin(chipSelect))
		{
			//error("Card failed, or not present");
		}

		char filename[] = "LOGGER00.CSV";
		for (uint8_t i = 0; i < 100; i++)
		{
			filename[6] = i / 10 + '0';
			filename[7] = i % 10 + '0';
			if (!SD.exists(filename))
			{
				// only open a new file if it doesn't exist
				logfile = SD.open(filename, FILE_WRITE);
				break;  // leave the loop!
			}
		}

		if (!logfile)
		{
			//error("couldnt create file");
		}

		Wire.begin();
		if (!RTC.begin())
		{
			logfile.println("RTC failed");

			logfile.println("millis,stamp,datetime,temp,press,vcc");

		}

	}

	void Start()
	{
		Reset();
		//pumpController.Start();
	}

	void PullStage()
	{
		pumpController.Start();
		Write();
	}

	void RestStage()
	{
		pumpController.Stop();
		sessions++;

		Write();
	}

	void Stop()
	{
		pumpController.Stop();
	}

	void Update()
	{
		lightController.Update();
		presureController.Update();
		pumpController.Update();

		// temperatureController
	}

	void Complete()
	{

	}

	void Write()
	{

		DateTime now;

		//delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

		//digitalWrite(greenLEDpin, HIGH);

		// log milliseconds since starting
		uint32_t m = millis();
		logfile.print(m);           // milliseconds since start
		logfile.print(", ");

		now = RTC.now();
		// log time

		logfile.print(now.unixtime()); // seconds since 1/1/1970
		logfile.print(", ");
		logfile.print('"');
		logfile.print(now.year(), DEC);
		logfile.print("/");
		logfile.print(now.month(), DEC);
		logfile.print("/");
		logfile.print(now.day(), DEC);
		logfile.print(" ");
		logfile.print(now.hour(), DEC);
		logfile.print(":");
		logfile.print(now.minute(), DEC);
		logfile.print(":");
		logfile.print(now.second(), DEC);
		logfile.print('"');


		  Serial.print(now.unixtime()); // seconds since 1/1/1970
		  Serial.print(", ");
		  Serial.print('"');
		  Serial.print(now.year(), DEC);
		  Serial.print("/");
		  Serial.print(now.month(), DEC);
		  Serial.print("/");
		  Serial.print(now.day(), DEC);
		  Serial.print(" ");
		  Serial.print(now.hour(), DEC);
		  Serial.print(":");
		  Serial.print(now.minute(), DEC);
		  Serial.print(":");
		  Serial.print(now.second(), DEC);
		  Serial.print('"');

		logfile.print(", ");
		logfile.print(sessions);
		logfile.print(", ");
		logfile.print("wrote data for file 2nd time");
		logfile.println();
		// if ((millis() - syncTime) < SYNC_INTERVAL) return;
		// syncTime = millis();

		logfile.flush();
	}

	void Reset()
	{
		Serial.println("Intialize pen test model");

		sessions = 0;

		lightController.Initialize();
		presureController.Initialize();
		pumpController.Reset();

	}

};

PenTest penTestModel;

bool testRunning = false;

Button mainButton = Button(7);

int PULL_DURATION = 3000;

int REST_DURATION = 5000;

bool sessionOn = false;

SimpleTimer timer;

void testComplete()
{

}

void setup()
{
	Serial.begin(9600);
	// Add your initialization code here

	// add event to a button
	//gEM.addListener( EventManager::kEventAnalog5, initializeNewTest );

	Serial.println("Pen Test Setup");

	mainButton.pressHandler(handleOnButtonDown);

	penTestModel.Reset();


}

void PenStateChange()
{
	if (sessionOn = !sessionOn)
	{
		timer.setTimeout(PULL_DURATION, PenStateChange);

		penTestModel.PullStage();
	}
	else
	{
		timer.setTimeout(REST_DURATION, PenStateChange);

		penTestModel.RestStage();
	}

	Serial.print("State Change: ");
	Serial.println(sessionOn);
}

void handleOnButtonDown(Button& b)
{
	Serial.println("start new project");

	testRunning = true;

	penTestModel.Start();

	PenStateChange();
}

// The loop function is called in an endless loop
void loop()
{

	mainButton.process();

	if (testRunning)
	{
		penTestModel.Update();
		timer.run();
	}
}

