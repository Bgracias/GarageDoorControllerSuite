//MAC Address og home ctrl: bgracias@gmail.com

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <esp_now.h>
//#include "DHT.h"
#include <SimpleTimer.h>

#define DEFAULT_LED_MODE false
//#define DEFAULT_Temperature 0
//#define DEFAULT_Humidity 0
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


//static uint8_t DHTPIN = 5;
static uint8_t led_door_state_pin = 19;
static uint8_t led_power_pin = 4;
static uint8_t power_switch_pin =  18;
const int buzzerPin = 23;
// Define LED and pushbutton state booleans
bool powerState = true;
bool doorOpenState = false;
bool doorWarningState = false;
bool isOpenTimerDone=false;
bool isMultiFlag=false;
//bool wifi_connected = 0;
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//DHT dht(DHTPIN, DHT22);
//DHT dht(DHTPIN, DHTTYPE);
SimpleTimer checkTempTimer;
//SimpleTimer testTimer;
//SimpleTimer showOpenTimer;

void showMessage(String line1, String line2="")
{
  // Display Numbers
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  if(line2!="")
  {
    //display.drawBitmap(90, 2, Alarm88, 8, 8, WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);
    //display.setTextColor(SSD1306_INVERSE);
    display.println(line1);
    display.setCursor(0,28);
    display.setTextSize(2);
    display.println(line2);
  }
  else
  {
    display.setCursor(0,28);
    display.setTextSize(2);
    display.print(line1);    
  }

  display.display();
  delay(30);
}

void Buzz(bool oneshot)
{
  int j=0;
  int jtimes=5;
  if(oneshot)
  {
    jtimes=1;
  }
  
  while(j++<jtimes)
  {
     int i=0;
  
    while(i++<10)
    {
      digitalWrite(buzzerPin,HIGH);
      delay(20);//wait for 1ms
      digitalWrite(buzzerPin,LOW);
      delay(20);//wait for 1ms
    }
    delay(500);  
  }
  
}

//NOW code
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// Formats MAC Address
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
// Called when data is received
{
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);

  // Make sure we are null terminated
  buffer[msgLen] = 0;

  // Format the MAC address
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);

  // Send Debug log message to the serial port
  Serial.printf("Received message from: %s - %s\n", macStr, buffer);

  // Check switch status
  if (strcmp("on", buffer) == 0)
  {
    doorOpenState=true;
    broadcast("endFlash");   
  }
  else if (strcmp("warn", buffer) == 0)
  {
    doorOpenState=true;
    doorWarningState = true;   
    broadcast("startFlash");
  }
  else
  {
    broadcast("endFlash");
    doorOpenState=false;
    doorWarningState = false;
    digitalWrite(led_door_state_pin,LOW);
  }
  //digitalWrite(led_power_pin, ledPowerState);
  if (strcmp("FLOODING",buffer) == 0)
  {
    showMessage("Basement/AC", "Flooded");
    Buzz(true);
  }
}


void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
// Called when data is sent
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void broadcast(const String &message)
// Emulates a broadcast
{
  Serial.println("Broadcasting ...");
  Serial.println(message);
  
  // Broadcast a message to every device in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());

  // Print results to serial monitor
  if (result == ESP_OK)
  {
    Serial.println("Broadcast message success");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    Serial.println("ESP-NOW not Init.");
  }
  else if (result == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (result == ESP_ERR_ESPNOW_INTERNAL)
  {
    Serial.println("Internal Error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Unknown error");
  }
}
//end Now code
void setup()
{
  Serial.begin(9600);
  pinMode(buzzerPin,OUTPUT);//initialize the buzzer pin as an output
  pinMode(led_power_pin, OUTPUT);
  digitalWrite(led_power_pin, HIGH);
  pinMode(led_door_state_pin, OUTPUT);
  digitalWrite(led_door_state_pin, LOW);
  pinMode(power_switch_pin, INPUT_PULLUP);
  //dht.begin();
 // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  Buzz(true);
  showMessage("Starting..");
  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW Broadcast Demo");

  // Print MAC address
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Disconnect from WiFi
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESP-NOW Init Success");
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    ESP.restart();
  }

  checkTempTimer.setInterval(1000*30);
  //testTimer.setInterval(1000*40);
  //delay(5000);  
}

void loop()
{
  byte switchState0 = digitalRead(power_switch_pin);  //read the state of the input pin (HIGH or LOW)
  if (switchState0 == LOW)  //if it is LOW then the switch is closed
  {
    
    if(isMultiFlag==false)
    {
      Serial.println("Switch is off"); //print a message
      //send message to door controller to stop monitoring
      isMultiFlag=true;
      broadcast("off");
      digitalWrite(led_power_pin, LOW);
      //Buzz(true);
    }
    showMessage("Automatic Garage Door","OFF");    
  }
  else
  {
    if(isMultiFlag)
    {
      Serial.println("Switch is On"); //print a message
      //send message to door controller to stop monitoring
      isMultiFlag=false;
      broadcast("on");
      digitalWrite(led_power_pin, HIGH);
       showMessage("Automatic Garage Door","ON");
    }
  }
  if(doorOpenState && doorWarningState)
  {
    Serial.println("Warning and is Open"); //print a message
     
    //digitalWrite(led_door_state_pin, (millis() / 1000) % 2);
    int cnt=0;
    while(cnt++<10)
    {
      digitalWrite(led_door_state_pin, HIGH);
      showMessage("Backing Car?", "STOP");
      Buzz(true);
      drawfillcircle();
      
      digitalWrite(led_door_state_pin, LOW);
      
    }
    
   
  }
  else if(doorOpenState)
  {
     Serial.println("Open only"); //print a message
     doorWarningState=false;
     digitalWrite(led_door_state_pin, (millis() / 1000) % 2); 
     if(switchState0 == LOW)
           showMessage("Garage Door","Disabled  (OPEN)");
     else
           showMessage("Garage Door","OPEN");
     
     //showOpenTimer.reset();
  }
  else
  {
    //Serial.println("Both state are false"); //print a message
      
    digitalWrite(led_door_state_pin,LOW);
  }
  
  if (checkTempTimer.isReady() && !doorOpenState) {                      
      /*Serial.println("Sending DHT Sensor Data");
      float hum = dht.readHumidity();
      float temp = dht.readTemperature(true);
      if (isnan(hum) || isnan(temp)) {
        Serial.println("Failed to read from DHT sensor!");
        showMessage("FAIL-T");
        checkTempTimer.reset();
        return;
      }
      Serial.print("Temperature: ");
      Serial.println(temp);
      Serial.print("Humidity: ");
      Serial.println(hum);
      */
      //showMessage("Temp:"+String(temp, 2),"Hum:"+String(hum, 2));

      if(doorOpenState)
      {
        Serial.println("Open");
        if(switchState0 == LOW)
           showMessage("Garage Door","Disabled  (Open)");
        else
           showMessage("Garage Door","Open");
      }
      else
      {
        Serial.println("Close");
        if(switchState0 == LOW)
           showMessage("Garage Door","Disabled  (Closed)");
        else
           showMessage("Garage Door","Closed");
      }
      //doorOpenState=doorOpenState==false?true:false;
      checkTempTimer.reset();                         
    }  
  
  

//  if (testTimer.isReady() ) {
//     doorWarningState=doorWarningState==false?true:false;
//    doorOpenState=true;
//    Serial.print("TestTimer ready again with doorWarningState=");                    
//   Serial.println(doorWarningState);                    
//   
//    testTimer.reset();  
//    //isWarningTimerDone=false;                       
//  }
  delay(1000);
}

void drawfillcircle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=3) {
    // The INVERSE color is used so circles alternate white/black
    display.fillCircle(display.width() / 2, display.height() / 2, i, SSD1306_INVERSE);
    display.display(); // Update screen with each newly-drawn circle
    delay(1);
  }
  
  //delay(2000);
}
