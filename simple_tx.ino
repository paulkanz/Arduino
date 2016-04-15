#include <RFM69.h>
#include <SPI.h>
//
//
#define NODEID      22  //  all devices on the same network should have unique value 2-255 (1 is for gateway)
#define NETWORKID   200 //  all the devices on the same network sould be set to the same
#define GATEWAYID   1
#define FREQUENCY   RF69_433MHZ
#define KEY         "Some16Characters"  //  Needs to be 16 characters and the same on all devices
#define LED         9
#define SERIAL_BAUD 115200
#define SENSORTYPE 1    //  Tempature and Humudity
#define VERSION  "0.0.1"
#define ACK_RETRIES 0   //  Only one retry will be sent
#define ACK_WAIT 50     //  time in ms
//
//
// boolean requestACK = false;
RFM69 radio;
//
//
void Blink (byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  analogWrite(PIN, 75);
  delay(DELAY_MS);
  analogWrite(PIN, 0);
}
//
//
int readVcc() {   // return vcc voltage in millivolts
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);             // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);  // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate in mV
  int t = (int)result;
  return t;
}
//
//
//  Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
  return 1.8 * celsius + 32;
}
//
//  dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);
  //
  //  factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;
  //
  //  (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}
//
//
void setup() {
  //
  //
  Serial.begin(SERIAL_BAUD);
  Serial.print("simple_tx powering up...running v");
  Serial.println(VERSION);
  Blink(LED, 1000);
  //
  //
  Serial.print("simple_tx startup at ");
  Serial.print((FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915));
  Serial.println("Mhz...");
  delay(20);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.encrypt(KEY);
}
//
//
void loop() {
  //
  //  fake numbers, real would come from a sensor
  /   could also use the built in temp sensor on RFM69:
  //  byte temperature =  radio.readTemperature(-1);
  float temperature = random(0, 45);
  float humidity = random(10, 99);
  float dewpoint = Fahrenheit(dewPoint(temperature, humidity));
  float vcc = readVcc() * .001;
  //
  //
  char pkt[28];
  //
  char tempfb[7];
  dtostrf(Fahrenheit(temperature), 5, 2, tempfb);
  //
  char humbuf[7];
  dtostrf(humidity, 5, 2, humbuf);
  //
  char dewbuf[7];
  dtostrf(dewpoint, 5, 2, dewbuf);
  //
  char battb[6];
  dtostrf(vcc, 4, 2, battb);
  //
  //  Need to use strings, since float will not work
  sprintf(pkt, "%d,%s,%s,%s,%s", SENSORTYPE, tempfb, humbuf, dewbuf, battb);
  //
  radio.sendWithRetry(GATEWAYID, (const void*)(&pkt), sizeof(pkt), ACK_RETRIES, ACK_WAIT);
  Serial.println(pkt);
  //
  //  blink led that packet sent
  Blink(LED, 30);
  //
  delay(60000);
}
//
//
