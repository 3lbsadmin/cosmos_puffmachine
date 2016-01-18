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

	int motorPin = 9;
	int speedControlPin = 5;
	int highFrequency = 255;
	int lowFrequency = 60;

	bool running = false;

public:
	PumpController()
	{
		pinMode(motorPin, OUTPUT);
	}

	void update()
	{
		int speedValue = analogRead(speedControlPin) / 4;

		if (speedValue != highFrequency)
		{
			///highFrequency = speedValue;
			//Serial.print("motor speed control value: ");
			//Serial.println(highFrequency);
		}
	}

	void pullState()
	{
		analogWrite(motorPin, highFrequency);
	}

	void lowState()
	{
		analogWrite(motorPin, lowFrequency);
	}

	void stop()
	{
		Reset();
		analogWrite(motorPin, 0);
	}

	void Reset()
	{
		running = false;
		highFrequency = 255;
		//pinMode( speedControlPinMode, INPUT );
		//analogWrite( pinMode, 124 );
	}
};

class PresureSensor
{
	int sensePinA = 0; // 2 on sensor
	int sensePinB = 1; // 4 on sensor

	int deltaPinAB;
	float pressdiff = 0;

	float maxPressure = 0;

public:
	PresureSensor()
	{
		// analogWrite(v4,255);
		// analogWrite(v3,0);
	}

	void Initialize()
	{

	}

	void puffStart()
	{
		maxPressure = 0;
	}

	void puffEnd()
	{
		maxPressure = 0;
	}

	float getMaxPressure()
	{
		return maxPressure;
	}

	void update()
	{
		deltaPinAB = analogRead(sensePinA) - analogRead(sensePinB);
		pressdiff = deltaPinAB * 4.33 / 70; //sensor is 70mV per psi, 1 byte is 4.33mV

		if (pressdiff > maxPressure)
		{
			maxPressure = pressdiff;
		}
	}
};

class PenLightSensor
{
public:

	const int STOPPED = 0;
	const int STARTED = 1;
	const int COMLETED = 2;
	const int FAILED = 3;

	int status = 0;

	int NO_LED_ON_TIME = 10000; // 10 seconds
	int FLASH_TIME = 1000; // 1 second
	int BATTERY_DEAD_FLASH_COUNT = 10;
	int OUTPUT_CUTOFF_FLASH_COUNT = 2;
	int PUFF_MAX_TIME = 5000;

	int flashCount = 0;
	int lightPinMode = 3;

	int flashes = 0;

	int lightValue = 0;
	bool _on = false;
	unsigned long _onTime = 0;
	unsigned long startTime = 0;
	unsigned long totalTime = 0;

	PenLightSensor()
	{
		pinMode(lightPinMode, INPUT);

	}

	void initialize()
	{

	}

	int getValue()
	{
		lightValue = analogRead(lightPinMode);
		return lightValue;
	}

	bool isOn()
	{
		return getValue() >= 30;
	}

	void start()
	{
		status = STARTED;
		_onTime = 0;
		startTime = millis();
		totalTime = 0;
	}

	void complete()
	{
		status = COMLETED;
	}

	void failed()
	{
		status = FAILED;
		Serial.println("failed test");
	}

	bool isFailed()
	{
		return ( status == FAILED );
	}

	bool isComplete()
	{
		return ( status == COMLETED );
	}

	void update()
	{

		Serial.print( "status:");
		Serial.println( status );

		if ( status == STARTED )
		{
			unsigned long currentMillis = millis();

			if (_onTime == 0 && (currentMillis - startTime) > NO_LED_ON_TIME)
			{
				//failed();
				return;
			}

			if (isOn())
			{
				_on = true;
				_onTime = millis();
			}
			else
			{
				if (_on)
				{
					_on = false;

					totalTime = millis() - _onTime;

					if (totalTime < FLASH_TIME)
					{
						// add flash
						flashes++;
					}
					else
					{
						if (flashes == 2 )
						{
						// two flash length = puff exceded;

						}
						else if ( flashes > 9 )
						{
							// 10 flashes mean battery dead
							// then dead battery

							//complete();
						}

						flashes = 0;

					}
				}
			}
		}
	}

	void stop()
	{
		status = STOPPED;
	}
};

class PenTest
{

	int totalPuffs = 0;

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

	}

	void setup()
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

	void start(int session)
	{
		/*logfile.print("Session:");
		logfile.print(session);

		logfile.print("date:");

		DateTime now;

		now = RTC.now();

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

		logfile.print("puff duration:");
		logfile.print("5000");

		logfile.println();

		//logfile.flush();*/

		reset();
		lightController.start();
	}

	void pullStage()
	{
		Serial.println("Moving to pull state");
		pumpController.pullState();
		presureController.puffStart();
	}

	void restStage()
	{

		totalPuffs++;
		pumpController.lowState();
		presureController.puffEnd();

		write();

		Serial.println("Moving to rest state");

	}

	void stop()
	{
		pumpController.stop();
		lightController.stop();
	}

	void update()
	{
		lightController.update();
		presureController.update();
		pumpController.update();

		if (lightController.isFailed() == true )
		{
			failed();
		}
		else if (lightController.isComplete() == true )
		{
			complete();
		}

	}

	void complete()
	{
		Serial.println( "test completed" );
		stop();
		write();
	}

	void failed()
	{
		Serial.println( "test failed" );
		stop();
		write();
	}

	void write()
	{

		//delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
		// log milliseconds since starting
		uint32_t m = millis();
		logfile.print(m);           // milliseconds since
		logfile.print(", ");

		/*logfile.print(now.unixtime()); // seconds since 1/1/1970
		 DateTime now;
		 now = RTC.now();

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
		 logfile.print('"');*/

		/*Serial.print(now.unixtime()); // seconds since 1/1/1970
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
		 Serial.print('"');*/

		//logfile.print(", ");
		logfile.print(totalPuffs);
		logfile.print(", ");
		logfile.print(presureController.getMaxPressure());
		logfile.println();
		// if ((millis() - syncTime) < SYNC_INTERVAL) return;
		// syncTime = millis();

		logfile.flush();

		Serial.println ( "write complete");

	}

	void reset()
	{
		Serial.println("Intialize pen test model");

		totalPuffs = 0;

		lightController.initialize();
		presureController.Initialize();
		pumpController.Reset();

	}

};

PenTest penTestModel;

bool testRunning = false;

Button onOffButton = Button(7);

int PULL_DURATION = 3000;
int REST_DURATION = 5000;

int PULL_TIMER_ID = 0;
int REST_TIMER_ID = 0;

bool puffIntervalOn = false;

int sessionCount = 0;

SimpleTimer timer;

PumpController pumpController;

void setup()
{
	Serial.begin(9600);
	Serial.println("Welcome to Pen Test Setup");

	onOffButton.pressHandler(handleOnButtonDown);

	penTestModel.reset();

}

void penStateChange()
{

	Serial.print ( "timer callback:");
	Serial.println( puffIntervalOn );

	if (puffIntervalOn = !puffIntervalOn)
	{
		PULL_TIMER_ID = timer.setTimeout(PULL_DURATION, penStateChange);
		penTestModel.pullStage();
	}
	else
	{
		REST_TIMER_ID = timer.setTimeout(REST_DURATION, penStateChange);
		penTestModel.restStage();
	}
}

void handleOnButtonDown(Button& b)
{

	if (!testRunning)
	{
		Serial.print("Start new pen test:");
		Serial.println(sessionCount++);
		testRunning = true;

		penTestModel.start(sessionCount);

		penStateChange();

	}
	else
	{
		Serial.print("end test:");
		Serial.println(sessionCount);

		testRunning = false;

		penTestModel.stop();

		timer.deleteTimer(PULL_TIMER_ID);
		timer.deleteTimer(REST_TIMER_ID);

	}
}

// The loop function is called in an endless loop
void loop()
{

	if (testRunning)
	{
		onOffButton.process();
		penTestModel.update();
		timer.run();
	}

}

