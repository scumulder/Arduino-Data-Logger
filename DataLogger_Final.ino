#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>

/*************************************************VARIABLES******************************************************/
#define DURATION 1                  //Number of hours you want this program to run once the button is pressed
#define LOG_INTERVAL 100            //Milliseconds between entries (reduce to take more/faster data)


#define SYNC_INTERVAL 1000          // millis between calls to flush() - to write data to the card
uint32_t syncTime = 0;              // time of last sync()

#define ECHO_TO_SERIAL 1            //Echo data to serial port
                                    //Easy method to turn serial printing on and off. Set to 0 if you don't want to print to the Serial Monitor

RTC_DS1307 RTC;                     //define the Real Time Clock object

const int chipSelect = 10;          //for the data logging shield, we use digital pin 10 for the SD cs line

const int pushbutton = 8;
const int done_light = 9;
/***All the variables for voltage reading function***/
const int voltage_in = A0;             //pin connected to the voltage divider output  
float voltage_raw;                     //raw 0 to 1023 integer reading from the ADCs
float voltage_reading;                 //voltage value between 0 and 5V
float voltage_true;                    //voltage value between 0 and 10V
uint32_t start_time;

//the logging file
File bhoomiFile;

LiquidCrystal lcd(2,3,4,5,6,7);

/****************************************************************************************************************/
/****************************************************SETUP*******************************************************/
void setup() 
{
  pinMode(pushbutton, INPUT);                 //Set pushbutton as Input 
  pinMode(voltage_in, INPUT);                 //Set voltage input
  pinMode(done_light, OUTPUT);                //Set the LED as an output
  digitalWrite(done_light, LOW);              //Make sure the LED is off when we start the program
  
  //chip select pin mode set within SD_INIT()
  
  Serial.begin(9600);                         //Initialize the serial communication
  lcd.begin(16, 2);                           //Initialize the LCD
  lcd.clear();                                //Clear screen and return cursor to home at startup

  #if ECHO_TO_SERIAL
    Serial.println("Press button to start");                                //Prompt user to press button
  #endif
  lcd.setCursor(0, 0);
  lcd.print("Press button");
  lcd.setCursor(0, 1);       
  lcd.print("to start..."); 
                                                       
  while(digitalRead(pushbutton)){};                                         //Wait for pushbutton input
  #if ECHO_TO_SERIAL
    Serial.println("(BUTTON PRESSED!)");                                    //Once pressed, display a message saying so
  #endif
  lcd.clear();
  lcd.print("BUTTON PRESSED!");
  delay(1000);                                                              //Display the message for a second
  lcd.clear();     
  
  SD_INIT();
  createFile();

  //connect to RTC
  Wire.begin();                                                     //Wire.begin: Initiate the Wire library and join the I2C bus as a master or slave.                                                      
  if (!RTC.begin())                                                 //RTC.begin initializes the internal RTC. It needs to be called before any other RTC library methods 
  {                                                                 //Assume that it returns 1 on success and 0 on failure
    bhoomiFile.println("RTC failed");
    lcd.println("RTC failed");
    #if ECHO_TO_SERIAL
      Serial.println("RTC failed");
    #endif  //ECHO_TO_SERIAL
  }
  bhoomiFile.println("Time (MM/DD/YYYY HH:MM:SS), Voltage (V)");        //If RTC initializes successfully...set this as a header in the file 
  #if ECHO_TO_SERIAL
    Serial.println("Time (MM/DD/YYYY HH:MM:SS), Voltage (V)");
  #endif //ECHO_TO_SERIAL

  start_time = millis();              //The very next step after this will be the data recording
                                      //Therefore, note down the time at this moment as the start time                                        
}
/****************************************************************************************************************/

/*************************************************LOOP***********************************************************/
void loop() 
{
    //Below: As long as current time - start time < required duration, keep recording the data
    for(start_time; (millis() - start_time) < (DURATION * 60 * 60 * 1000L); )
//To have record time in HOURS: (DURATION * 60 * 60 * 1000L)
      
    {
      doTheThing();
    }
    lcd.clear();
    while(1)
    {

      #if ECHO_TO_SERIAL
        Serial.println("ALL DONE");           //Once the set amount of time has passed, display message saying so
      #endif //ECHO_TO_SERIAL

      digitalWrite(done_light, HIGH);
      //lcd.home();
      lcd.print("ALL DONE");
      lcd.home();      
    }                       
}
/****************************************************************************************************************/

/************************************************FUNCTIONS*******************************************************/

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  while(1);
}

void SD_INIT()
{
  Serial.print("Initializing SD card...");
  pinMode(chipSelect, OUTPUT);
  
  //Check if the card is present and can be initialized:
  if (!SD.begin(chipSelect))                                      //SD.begin initializes the SD library and card; Returns 1 on success, 0 on failure  
  {
    lcd.println("SD Card failed");
    delay(30);
    error("Card failed, or not present");                         //Error function displays the error message if SD.begin fails and returns 0
    return; 
  }
  Serial.println("card initialized.");                            //If SD.begin is successful
}

void createFile()
{
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++)                               //Make a new file every time the Arduino starts up
  {                                                               //Goes from 00 to 99
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (!SD.exists(filename))                                     //SD.exists() tests whether a file or directory exists on the SD card
    {                                                             //It returns true if the file or directory exists, false if not
      //only open a new file if it doesn't exist
      bhoomiFile = SD.open(filename, FILE_WRITE);                 /*SD.open() opens a file on the SD card. If the file is opened for writing, 
                                                                  it will be created if it doesn't already exist (but the directory containing it must already exist).
                                                                  It returns a File object referring to the opened file; if the file couldn't be opened, 
                                                                  this object will evaluate to false in a boolean context*/
                                                                  //FILE_WRITE enables read and write access to the file
      break;  // leave the loop!
    }
  }

  if (!bhoomiFile)                                                //If file couldn't be opened/created 
  {
    error("couldn't create file");
  }
  Serial.print("Logging to: ");                                   //Otherwise, display the name of the file generated  
  Serial.println(filename);
  lcd.println(filename);
  delay(1300);
  lcd.clear();
}

void doTheThing()
{
  DateTime now;
  
    //delay for the amount of time we want between readings
    delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));
        
    //fetch the time
    now = RTC.now();
    bhoomiFile.print("  ");                  //This space is important. If you remove this, the seconds will not be recorded in the log file
    bhoomiFile.print(now.month(), DEC);
    bhoomiFile.print("/");
    bhoomiFile.print(now.day(), DEC);
    bhoomiFile.print("/");
    bhoomiFile.print(now.year(), DEC);
    bhoomiFile.print(" ");
    bhoomiFile.print(now.hour(), DEC);
    bhoomiFile.print(":");
    bhoomiFile.print(now.minute(), DEC);
    bhoomiFile.print(":");
    bhoomiFile.print(now.second(), DEC);
    bhoomiFile.print("");
    #if ECHO_TO_SERIAL
      Serial.print(now.month(), DEC);
      Serial.print("/");
      Serial.print(now.day(), DEC);
      Serial.print("/");
      Serial.print(now.year(), DEC);
      Serial.print(" ");
      Serial.print(now.hour(), DEC);
      Serial.print(":");
      Serial.print(now.minute(), DEC);
      Serial.print(":");
      Serial.print(now.second(), DEC);
      Serial.print("");
    #endif //ECHO_TO_SERIAL
    
    voltage_raw = analogRead(voltage_in);
    voltage_reading = (voltage_raw * 5.0)/1024.0;
    voltage_true = (voltage_reading)*((9980.0 + 9950.0)/9950.0);
    
    //Log the voltage reading
    bhoomiFile.print(", ");    
    bhoomiFile.print(voltage_reading, 4);               //for variables of datatype 'float', the number in the print() funtion specifies
                                                        //the number of digits to be printed out
    
    //Print to LCD
    lcd.setCursor(0,0);
    lcd.print("Voltage = ");
    lcd.print(voltage_reading, 5);
    lcd.print("V");
      
    #if ECHO_TO_SERIAL
      Serial.print(", ");   
      Serial.print(voltage_reading, 4);
    #endif //ECHO_TO_SERIAL
  
    bhoomiFile.println();
    #if ECHO_TO_SERIAL
      Serial.println();
    #endif // ECHO_TO_SERIAL
    
     
    //When you use file.write(), it doesn't write to the card until you flush() or close(). 
    //Whenever you open a file, be sure to close it to save your data. 
    if ((millis() - syncTime) < SYNC_INTERVAL) return;
    syncTime = millis();
    bhoomiFile.flush();               //flush() ensures that any bytes written to the file are physically saved to the SD card
}
/****************************************************************************************************************/

