/*
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  Single shot read - Ask the reader to tell us what tags it currently sees. And it beeps!

  If using the Simultaneous RFID Tag Reader (SRTR) shield, make sure the serial slide
  switch is in the 'SW-UART' position.
*/

#include <SoftwareSerial.h> //Used for transmitting to the device

SoftwareSerial softSerial(2, 3); //RX, TX

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module
RFID nano; //Create instance

#define BUZZER1 9
//#define BUZZER2 0 //For testing quietly
#define BUZZER2 10
int afstand = 0;
int rssi = 0;
int som = 0;

const int aantalMetingen = 10;
int aantal = 0;

const int aantalMeetPunten = 20;
int meetPunt = 0;

double waarden[aantalMeetPunten][aantalMetingen + 2]; //0: afstand, 1: gemiddelde,2-... meetpunten
void setup()
{
  Serial.begin(115200);

  pinMode(BUZZER1, OUTPUT);
  pinMode(BUZZER2, OUTPUT);

  digitalWrite(BUZZER2, LOW);
  while (!Serial);
  Serial.println();
  Serial.println("Initializing...");

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println("Module failed to respond. Please check wiring.");
    while (1); //Freeze!
  }

  nano.setRegion(REGION_EUROPE); //Set to Europe

  nano.setReadPower(500); //5.00 dBm. Higher values may cause USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  nano.startReading();

}

void loop()
{ while (meetPunt < aantalMeetPunten && afstand >= 0) {
    Serial.println(F("geef de afstand van antenne tot tag in [mm]"));
    while (!Serial.available()); //Wait for user to send a character    
    afstand = Serial.parseInt(); //Throw away the user's character
    if (afstand > 0) {
      Serial.print("meetafstand: ");
      Serial.println(afstand);
      aantal = 0;
      som = 0;
      waarden[meetPunt][0] = (int)afstand;

      do {
        if (nano.check() == true) //Check to see if any new data has come in from module

        {
          byte responseType = nano.parseResponse(); //Break response into tag ID, RSSI, frequency, and timestamp

          if (responseType == RESPONSE_IS_KEEPALIVE)
          {
            Serial.println(F("Scanning"));
          }
          else if (responseType == RESPONSE_IS_TAGFOUND)
          {
            int rssi = nano.getTagRSSI();


            //Beep! Piano keys to frequencies: http://www.sengpielaudio.com/KeyboardAndFrequencies.gif
            //  tone(BUZZER1, 2093, 150); //C
            //  delay(150);
            //  tone(BUZZER1, 2349, 150); //D
            //  delay(150);
            //  tone(BUZZER1, 2637, 150); //E
            //  delay(150);

            //Print EPC
            Serial.print(F(" rssi["));
            Serial.print(rssi);
            Serial.println(F("]"));

            waarden[meetPunt][2 + aantal] = rssi;
            som = som + rssi;

            Serial.println(aantal);
            aantal++;
          }
        }
        delay(100);
      }
      while (aantal < aantalMetingen); //doorloop dit 10 maal
      waarden[meetPunt][1] = som / aantalMetingen; // gemiddelde waarde
      Serial.print("gemiddelde waarde op afstand ");
      Serial.print(afstand);
      Serial.print(" mm is: ");
      Serial.println(waarden[meetPunt][1]);

      printMeting(meetPunt);
      meetPunt++; // naar volgende meting
    }
    else {
      Serial.println("verkeerde invoer en dus eindigen van programma. Gemeten RssiMeetbank wordt uitgeprint.");
    }
  }
  afstand = 0;
  meetPunt = 0;
  nano.stopReading();
  printMetingen();
}
//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(softSerial); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while (!softSerial); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while (softSerial.available()) softSerial.read();

  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
    nano.stopReading();

    Serial.println(F("Module continuously reading. Asking it to stop..."));

    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    softSerial.begin(115200); //Start software serial at 115200

    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}

void printMeting(int meetPunt) {
  for (int i = 0; i < aantalMetingen + 2; i++) {
    Serial.print(waarden[meetPunt][i]);
    Serial.print('\t');
  }
  Serial.println("");
}
void printMetingen() {
  for (int i = 0; i < aantalMeetPunten; i++) {
    printMeting(i);
    Serial.println("");
  }
}

