#include <ESP8266WiFi.h>
#include <SocketIoClient.h>
#include "DHT.h"
#include <time.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

static const char ntpServerName[] = "id.pool.ntp.org";
const int timeZone = 1;

WiFiUDP Udp;
unsigned int localPort = 8888; 

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

#define trigPin 4
#define echoPin 5
#define DHTPIN 12
#define rainsensor 13

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

SocketIoClient webSocket;

long duration;
int distance;

float suhu;
float kelembapan;
int jarak;
int hujan;

int old_jarak = 0;
float old_suhu= 0;
float old_kelembapan = 0;
int old_hujan = 1;

char waktu[30];

char suhu_udara[10];
char kelembapan_udara[10];
char jarak_air[10];
char status_hujan[10];

char jam[5];
char menit[5];
char detik[5];


char tanggal[5];
char bulan[5];
char tahun[5];

char result[100];

uint64_t messageTimestamp;
//menerima pesan dari server
void messageEventHandler(const char * payload, size_t length) {
  Serial.printf("got message: %s\n", payload);
}

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);
  pinMode(rainsensor, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); 
  

  dht.begin();
  
  Serial.begin(115200);
  WiFi.begin("CHIKO", "dionkoten97");
  Serial.print("Connecting");
  // menghubungkan wifi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP() );

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
 
  // server address, port and URL
  webSocket.begin("192.168.1.10", 3000);

  // event handler for the event message
  webSocket.on("reply", messageEventHandler);
}

time_t prevDisplay = 0;

void loop() {

  
  webSocket.loop();

      getValue();
    
  uint64_t now = millis();
  if (now - messageTimestamp > 30000) {
    messageTimestamp = now; 
    sendUptime();
  }else if(suhu != old_suhu || kelembapan != old_kelembapan || jarak != old_jarak || hujan != old_hujan){
             messageTimestamp = now; 
             old_suhu = suhu;
             old_kelembapan = kelembapan;
             old_jarak = jarak;
             Serial.println(jarak);
             old_hujan = hujan;
//             delay(2000);
             sendUptime();    
    }

}
void getValue(){
  calulateTem();
  jarak = calculateDistance();
  hujan = digitalRead(rainsensor);
}

void sendUptime()
{
    prevDisplay = now();
  digitalClockDisplay();
   
  //  convert to char
  dtostrf(suhu, 1, 2, suhu_udara);
  dtostrf(kelembapan, 1, 2, kelembapan_udara);
  dtostrf(jarak, 1, 0, jarak_air);
  dtostrf(hujan, 1, 0, status_hujan);

  for ( int i = 0; i < sizeof(result);  ++i ) {
    result[i] = (char)0;
  }


  strcat(result, "\"{'suhu_udara' : ");
  strcat(result, "'");
  strcat(result, suhu_udara);
  strcat(result, "'");
  strcat(result, ",'kelembapan_udara' : ");
  strcat(result, "'");
  strcat(result, kelembapan_udara);
  strcat(result, "'");
  strcat(result, ",'jarak_air' : ");
  strcat(result, "'");
  strcat(result, jarak_air);
  strcat(result, "'");
  strcat(result, ",'status_hujan' : ");
  strcat(result, "'");
  strcat(result, status_hujan);
  strcat(result, "'");
  strcat(result, ",'waktu' : ");
  strcat(result, "'");
  strcat(result, waktu);
  strcat(result, "'");
  strcat(result, "}");
  strcat(result, "\""); //end
  
  webSocket.emit("send_data_sensor", result);

   memset(waktu, 0, sizeof(waktu));

   digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
}

float calculateDistance() {
    delay(500);  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH); // Reads the echoPin, returns the sound wave travel time in microseconds
  distance = duration * 0.034 / 2;
  
   
  return distance;
}

float calulateTem() {
  kelembapan = dht.readHumidity();
  suhu = dht.readTemperature();
  
}


void digitalClockDisplay()
{

  
    int _jam = hour() + 7 ;
    int _menit = minute();
    int _detik = second() ;
    int _tanggal = day();
    int _bulan = month(); 
    int _tahun= year();

      
  //   Serial.print(_bulan);
  
  
    dtostrf(_jam, 1, 0, jam);
    dtostrf(_menit, 1, 0, menit);
    dtostrf(_detik, 1, 0, detik);
    
  dtostrf(_tanggal, 1, 0, tanggal);
  dtostrf(_bulan, 1, 0, bulan);
  dtostrf(_tahun, 1, 0, tahun);


   if(_tanggal < 10){
    strcat(waktu,"0");
   }
   strcat(waktu,tanggal);
   strcat(waktu,"/");
    if(_bulan < 10){
    strcat(waktu,"0");
   }
   strcat(waktu,bulan);
   strcat(waktu,"/");
    if(_tahun < 10){
    strcat(waktu,"0");
   }
   strcat(waktu,tahun);
   strcat(waktu," ");
    if(_jam < 10){
    strcat(waktu,"0");
   }
   strcat(waktu,jam);
   strcat(waktu,":");
    if(_menit < 10){
    strcat(waktu,"0");
   }
   strcat(waktu,menit);
   strcat(waktu,":");
    if(_detik < 10){
    strcat(waktu,"0");
   }
   strcat(waktu,detik);

}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
