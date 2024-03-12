//MAC Address: bgracias@gmail.com

#include <SimpleTimer.h>
#include <WiFi.h>
#include <esp_now.h>
//#include "RMaker.h"
//#include "WiFi.h"
//#include "WiFiProv.h"
//#include <wifi_provisioning/manager.h>
#define S   OUND_PWM_CHANNEL   0
#define SOUND_RESOLUTION    8 // 8 bit resolution
#define SOUND_ON            (1<<(SOUND_RESOLUTION-1)) // 50% duty cycle
#define SOUND_OFF           0                         // 0% duty cycle

// defines pins numbers
const int echoPin = 12;
const int trigPin = 13;
const int buzzerPin = 14;
const int garagePin=33;
String lastMessageSent="";

String offtxt="off";

//broadcast vars

const char* soft_ap_ssid="esp32";
const char* soft_ap_pwd= "xx";
 
// defines variables
long duration;
int distance;
int openDistance=50;
const char *service_name = "PROV_12345";
const char *pop = "1234567";

static uint8_t gpio_reset = 0;
SimpleTimer timeToClose;
SimpleTimer timeToCheck;
bool isGarageOpenFlag=false;
bool wifi_connected = 0;
bool doorSensorState=true;
bool timeToClosePaused=false;
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
  Serial.printf("Received Power message from: %s - %s\n", macStr, buffer);

  // Check switch status
  if (strcmp("on", buffer) == 0)
  {
    doorSensorState = true;    
  }
  else if(strcmp("off",buffer) == 0)
  {
    doorSensorState = false;
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
  Serial.println("Broadcasting...");
  
  // Broadcast a message to every device in range
  //uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t broadcastAddress[] = {0xF0, 0x08, 0xD1, 0xD1, 0x8E, 0xCC};
  
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
    lastMessageSent=message;
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
void setup()
{
  Serial.begin(9600);
  doorSensorState=true;

  timeToCheck.setInterval(5000);
  
  timeToClose.setInterval(1000*180);
  timeToClosePaused=false;
  //delay(5000);
  isGarageOpenFlag=false;


  pinMode(buzzerPin,OUTPUT);//initialize the buzzer pin as an output
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(garagePin,OUTPUT);
  // Set ESP32 in STA mode to begin with
  //WiFi.mode(WIFI_STA);
  //WiFi.mode(WIFI_MODE_APSTA);
   WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(soft_ap_ssid,soft_ap_pwd,1);
  Serial.println("ESP32 wifi softAP ip ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
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
}
bool IsGarageOpen()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  Serial.printf("Distance:");
  Serial.println(distance);
  distance= duration*0.034/2;
  if(distance > 0 && distance < openDistance)
  {
    return true;
  }
  else
  {
    return false;
  }
  
}
void CloseGarage()
{
    digitalWrite(garagePin,HIGH);
    delay(1000); 
    digitalWrite(garagePin,LOW);  
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
    delay(1000);  
  }
  
}

void loop()
{
  if (timeToCheck.isReady() ) {                    
    if(doorSensorState){
      Serial.println("Checking distance");

      if(IsGarageOpen())
      {
        Buzz(true);
        
        if(!isGarageOpenFlag)
        {
          Serial.println("Garage is Open timer set");
          broadcast("on");
        
          timeToClose.reset();
          timeToClosePaused=false;
          isGarageOpenFlag=true;
        }
        else
        {
          if(timeToClose.isReady() && doorSensorState && isGarageOpenFlag && !timeToClosePaused)
          {
            Serial.println("Garage closing");
            broadcast("warn");
            Buzz(false);
            delay(5000);
            
            CloseGarage(); 
            broadcast("off");  
            delay(500);
            isGarageOpenFlag=false;
            timeToClosePaused=true;
          }
        }
      }
      else
      {
        //safer to keep false when errorneously triggered
        Serial.println("Forcing cancellation,if any. Curent isgarageopenFlag is ");
        Serial.println(isGarageOpenFlag);
        if(isGarageOpenFlag){
          Serial.println("Garage is Open timer set");
          if(lastMessageSent!=offtxt)
            broadcast("off");
          //timeToClose.reset();
          isGarageOpenFlag=false;
        } 
      }
    
    
    } 
    else
    {
          Serial.println("Garage Sensor is overridden. Not doing anything.");      
    }
    timeToCheck.reset();                       
  }

}
