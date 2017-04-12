#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <avr/sleep.h>

// STEVEE_GPS for USMA Astronomy Club
// Based on 
// Ladyada's logger, modified by Bill Greiman to use the SdFat library
// Further modified by
// Eric Myers <Eric.Myers@usma.edu> - 11 April 2017

// Pin to use to signal the cutaway system to drop the balloon.  Don't use pin 8!
#define CUTAWAY_PIN 9
// Time (in minutes) until we abort the mission and pull that pin high
#define CUTAWAY_TIMEOUT 55
// Altitude (in meters) at which we will cut away
#define CUTAWAY_ALTITUDE 304.8
// Dwell time (in milliseconds) to leave CUTAWAY_PIN high
#define CUTAWAY_DWELL 3000

// This code shows how to listen to the GPS module in an interrupt
// which allows the program to have more 'freedom' - just parse
// when a new NMEA sentence is available! Then access data when
// desired.
//
// Tested and works great with the Adafruit Ultimate GPS Shield
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/
// Pick one up today at the Adafruit electronics shop 
// and help support open source hardware & software! -ada
// Fllybob added 10 sec logging option
SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  true
/* set to true to only log to SD when GPS has a fix, for debugging, keep it false */
#define LOG_FIXONLY false  

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy


// Set the pins used
#define chipSelect 10
#define ledPin 7

File logfile;

uint32_t timer = 0;
float AltitudeMeters = 0.0;
float ElapsedTime = 0.0;
bool did_cutaway = false;

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink out an error code
void error(uint8_t errno) {
  /**
  if (SD.errorCode()) {
   putstring("SD error: ");
   Serial.print(card.errorCode(), HEX);
   Serial.print(',');
   Serial.println(card.errorData(), HEX);
   }
   /**/
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
      delay(500);
    }
    //for (i=errno; i<10; i++) {
      delay(2000);
    //}
  }
}

void setup() {
  // for Leonardos, if you want to debug SD issues, uncomment this line
  // to see serial output
  //while (!Serial);

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("\r\nUltimate GPSlogger Shield");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
    
   // Setup the pin used to signal to the cutaway system
   pinMode(CUTAWAY_PIN, OUTPUT);
   digitalWrite(CUTAWAY_PIN, LOW); 

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  ///if (!SD.begin(chipSelect, 11, 12, 13)) {
  if (!SD.begin(chipSelect)) {      // if you're using an UNO, you can use this line instead
    Serial.println("Card init. failed!");
    error(2);
  }
  
  char filename[15];
  strcpy(filename, "GPSLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename);
    error(3);
  }
  
  Serial.print("Writing to log file "); 
  Serial.println(filename);

  // connect to the GPS at the desired rate
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For logging data, we don't suggest using anything but either RMC only or RMC+GGA
  // to keep the log files at a reasonable size
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 100 millihertz (once every 10 seconds), 1Hz or 5Hz update rate

  // Turn off updates on antenna status, if the firmware permits it
  GPS.sendCommand(PGCMD_NOANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);

  Serial.println("Ready!");
}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  #ifdef UDR0
      if (GPSECHO)
        if (c) UDR0 = c;  
      // writing direct to UDR0 is much much faster than Serial.print 
      // but only one character can be written at a time. 
  #endif
}


void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } 
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

/* Activate the cut-away circuit */

void cutAway(char* msg){
   Serial.println(msg);
   digitalWrite(CUTAWAY_PIN, HIGH);  
   did_cutaway = true;
   delay(CUTAWAY_DWELL);        
   digitalWrite(CUTAWAY_PIN, LOW);   
}



void loop() {
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
  }
  //Serial.print('x');
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    
    // Don't call lastNMEA more than once between parse calls!  Calling lastNMEA 
    // will clear the received flag and can cause very subtle race conditions if
    // new data comes in before parse is called again.
    char *stringptr = GPS.lastNMEA();
    
    if (!GPS.parse(stringptr)) {  // this also sets the newNMEAreceived() flag to false
      Serial.println("GPS.parse failed!");
      return;  // we can fail to parse a sentence in which case we should just wait for another
    }

    // Sentence parsed! 
    Serial.println("OK");
    if (LOG_FIXONLY && !GPS.fix) {
      Serial.print("No Fix");
      return;
    }

    // Rad. lets log it!
    //Serial.println("Log it.");

    uint8_t stringsize = strlen(stringptr);
    if (stringsize != logfile.write((uint8_t *)stringptr, stringsize))    //write the string to the SD file
        error(4);
    if (strstr(stringptr, "RMC") || strstr(stringptr, "GGA"))   logfile.flush();
    Serial.println();
  
    // Check the timer.  Abort if out of time

    ElapsedTime = (millis()-timer)/1000.0;
    Serial.print("Time: ");
    Serial.println(ElapsedTime);
    if( (ElapsedTime > CUTAWAY_TIMEOUT*60) && !did_cutaway ) {
        cutAway("Timed out. ");
    }
  /**/
    if( GPS.fix ){// cannot test altitude without a fix
      AltitudeMeters = GPS.altitude;
      Serial.print("Altitude (m): ");
      Serial.println(AltitudeMeters);
      if( (AltitudeMeters > CUTAWAY_ALTITUDE) && !did_cutaway) {
        cutAway("Above ceiling. ");
      }
    }
  /**/
  }
}


/* End code */

