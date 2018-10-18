#include <LCD5110_Graph.h>
#include <Wire.h>

// LCD parameters
LCD5110 myGLCD(8,9,10,12,11); // (5,4,3,1,2)

extern unsigned char SmallFont[];
extern unsigned char BigNumbers[];

// Vario Parameteres

const byte led = 13;

unsigned int calibrationData[7];
unsigned long time = 0;

float toneFreq, toneFreqLowpass, pressure, lowpassFast, lowpassSlow ;

int ddsAcc;

float sea_press = 1013.7;

float alt1 = 0; 



// Setup begin


void setup()
{
 // vario Setup
  Wire.begin();
  Serial.begin(115200);
  setupSensor();
  
  pressure = getPressure();
  lowpassFast = lowpassSlow = pressure;  // Initiate Pressure
 
 //LCD Setup
  myGLCD.InitLCD();
  myGLCD.setContrast(57);
  myGLCD.setFont(SmallFont);

 // randomSeed(analogRead(0));

  pinMode(7, OUTPUT);
  digitalWrite(7,LOW);
}




void loop()
{
  // Vario leep
  pressure = getPressure();
  
  float pressure0 = pressure/100; // for altimeter use
  
  lowpassFast = lowpassFast + (pressure - lowpassFast) * 0.1;
  lowpassSlow = lowpassSlow + (pressure - lowpassSlow) * 0.05;
  
  toneFreq = (lowpassSlow - lowpassFast) * 50;
  
  toneFreqLowpass = toneFreqLowpass + (toneFreq - toneFreqLowpass) * 0.1;
   
  toneFreq = constrain(toneFreqLowpass, -500, 500);
  
  ddsAcc += toneFreq * 100 + 2000;
  
  if (toneFreq < 0 || ddsAcc > 0) 
  {
    tone(2, toneFreq + 510);  
  }
  else
  {
    noTone(2);
  }
  
  ledOff(); 
  while (millis() < time);        //loop frequency timer
  time += 20;
  ledOn();
  // Altimeter 
     long T2,dT1;
     float temprature, altimeter;
     T2 = getData(0x50, 1);
     dT1 = T2 - ((long)calibrationData[5] << 8);
     temprature = (2000 + (((int64_t)dT1 * (int64_t)calibrationData[6]) >> 23)) / (float)100;
  
     altimeter = ((pow((sea_press / pressure0), 1/5.257) - 1.0) * (temprature + 273.15)) / 0.0065;
     Serial.println(altimeter);
  
  // Display
  String str1,str2;
  int val = altimeter;

  float c_rate = alt1 - altimeter;

   str1 = String(abs(altimeter));
   str2 = String(c_rate);     
    myGLCD.setFont(SmallFont);
    myGLCD.print("Altitude", 0, 0);
    myGLCD.drawLine(0,10,84,10);
    myGLCD.setFont(BigNumbers);
    myGLCD.print(str1,1,18);
    myGLCD.setFont(SmallFont);
    myGLCD.print(str2,60, 0);

    
  
    myGLCD.update();
    delay(50);
    alt1 = altimeter;
   
}


// Vario Functions

long getPressure()
{
  long D1, D2, dT, P;
  float TEMP;
  int64_t OFF, SENS;
 
  D1 = getData(0x48, 10);
  D2 = getData(0x50, 1);

  dT = D2 - ((long)calibrationData[5] << 8);
  TEMP = (2000 + (((int64_t)dT * (int64_t)calibrationData[6]) >> 23)) / (float)100;
  OFF = ((unsigned long)calibrationData[2] << 16) + (((int64_t)calibrationData[4] * dT) >> 7);
  SENS = ((unsigned long)calibrationData[1] << 15) + (((int64_t)calibrationData[3] * dT) >> 8);
  P = (((D1 * SENS) >> 21) - OFF) >> 15;
  
  //Serial.println(TEMP);
  Serial.println(P);
  
  return P;
}


long getData(byte command, byte del)
{
  long result = 0;
  twiSendCommand(0x77, command);
  delay(del);
  twiSendCommand(0x77, 0x00);
  Wire.requestFrom(0x77, 3);
  if(Wire.available()!=3) Serial.println("Error: raw data not available");
  for (int i = 0; i <= 2; i++)
  {
    result = (result<<8) | Wire.read(); 
  }
  return result;
}


void setupSensor()
{
  twiSendCommand(0x77, 0x1e);
  delay(100);
  
  for (byte i = 1; i <=6; i++)
  {
    unsigned int low, high;

    twiSendCommand(0x77, 0xa0 + i * 2);
    Wire.requestFrom(0x77, 2);
    if(Wire.available()!=2) Serial.println("Error: calibration data not available");
    high = Wire.read();
    low = Wire.read();
    calibrationData[i] = high<<8 | low;
    Serial.print("calibration data #");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println( calibrationData[i] ); 
  }
}


void twiSendCommand(byte address, byte command)
{
  Wire.beginTransmission(address);
  if (!Wire.write(command)) Serial.println("Error: write()");
  if (Wire.endTransmission()) 
  {
    Serial.print("Error when sending command: ");
    Serial.println(command, HEX);
  }
}


void ledOn()
{
  digitalWrite(led,1);
}


void ledOff()
{
  digitalWrite(led,0);
}
