#include <SoftwareSerial.h> 								  //Used for transmitting to the device
SoftwareSerial softSerial(2, 3); 							//RX, TX

#include "SparkFun_UHF_RFID_Reader.h" 				//Library for controlling the M6E Nano module
RFID nano; 													          //Create instance

String EPC = "";
byte tagDataBytes = 0;
void(* resetFunc) (void) = 0; 								//declare reset function @ address 0

void setup()
{
  Serial.begin(9600);										      //Init of Serial with baudrate of 9600
  while (!Serial); 											      //Wait for the serial port to come online

  if (setupNano(38400) == false) 							//Configure nano to run at 38400bps
  {
    stuurFout();
    delay(1000); 											        //wait
    Serial.println(nano.msg[0]);							//print response/msg
    resetFunc();  											      //call reset
  }

  nano.setRegion(REGION_EUROPE); 							//Set to rules of Europe

  nano.setReadPower(2000); 									  //20.00 dBm. Highest possible value in Europe
  
  nano.startReading(); 										    //Begin scanning for tags
}

void loop()
{
  if (nano.check() == true) 								  //Check to see if any new data has come in from module
  {
    byte responseType = nano.parseResponse(); //Break response into tag ID, RSSI, frequency, and timestamp
    if (responseType == RESPONSE_IS_TAGFOUND)
    {
															                //If we have a full record we can pull out the fun bits
      int rssi = nano.getTagRSSI(); 					//Get the RSSI for this tag read;

      byte tagEPCBytes = nano.getTagEPCBytes(); //Get number of EPC bytes from response

      for (byte x = 0 ; x < tagEPCBytes ; x++)//for loop to get EPC of readed tag
      {
        if (x == tagEPCBytes / 2) { 					//split EPC in two values of 6 bytes
          EPC = EPC + ',';
        }
        EPC = EPC + String(nano.msg[31 + tagDataBytes + x]);
      }
      Serial.print(EPC);									    //send splitted EPC
      Serial.print(',');									    //split EPC and RSSI with comma
      Serial.print(rssi);									    //send RSSI value of tag
      Serial.println(';');									  //end transmission with semicolon

    }
  }
  EPC = "";													          //clear value of EPC
}

//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(softSerial); 							      //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); 						    //For this test, assume module is already at our desired baud rate
  while (!softSerial); 								        //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while (softSerial.available()) softSerial.read();

  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
    nano.stopReading();
    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    softSerial.begin(115200); 						    //Start software serial at 115200

    nano.setBaud(baudRate); 						      //Tell the module to go to the chosen baud rate. Ignore the response msg

    softSerial.begin(baudRate); 					    //Start the software serial port, this time at user's chosen baud rate
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false);//Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); 							        //Set protocol to GEN2

  nano.setAntennaPort(); 							        //Set TX/RX antenna ports to 1

  return (true); 									            //We are ready to rock
}

void stuurFout() {
  Serial.print('#'); 								          // fault symbol
  Serial.print(';'); 								          // end data with semicolon
}
