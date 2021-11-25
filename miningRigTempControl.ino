#include <ESP8266WiFi.h>
#include <DHT.h>

#define fanOutput 16
#define MOBO_DHTPin 5
#define GPU_DHTPin 4
#define PSU_DHTPin 0

String header;

float temperature[3] = {0,0,0};
float humidity[3] = {0,0,0};
float lowTemp, avgTemp, highTemp, lowHum, avgHum, highHum;
int fanState = 1;
int freezing = 0;

char* ssid = "stuff2";
char* password = "yeanoperoo";

WiFiServer server(80);
DHT MOBO_DHT(MOBO_DHTPin, DHT11);
DHT GPU_DHT(GPU_DHTPin, DHT11);
DHT PSU_DHT(PSU_DHTPin, DHT11);

void setup() {
  pinMode(fanOutput, OUTPUT);
  digitalWrite(fanOutput, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lowTemp = 0;
  highTemp = 0;
  lowHum = 0;
  highHum = 0;
  
  server.begin();
  MOBO_DHT.begin();
  GPU_DHT.begin();
  PSU_DHT.begin();
}

void loop(){
  temperature[0] = MOBO_DHT.readTemperature();
  humidity[0] = MOBO_DHT.readHumidity();
  temperature[1] = GPU_DHT.readTemperature();
  humidity[1] = GPU_DHT.readHumidity();
  temperature[2] = PSU_DHT.readTemperature();
  humidity[2] = PSU_DHT.readHumidity();

  avgTemp = 0;
  avgHum = 0;

  for(int i = 0; i < 3; i++) {
    //if any temp sensor is freezing, we ignore the average
    if (temperature[i] <= 5) {
      avgTemp = 0;
      freezing = 1;
      break;
    } else {
      freezing = 0;
    }
  }

  //need separate loop in case break is called
  for(int i = 0; i < 3; i++) {
    avgTemp = avgTemp + temperature[i];
    avgHum = avgHum + humidity[i];
    
    if (temperature[i] < lowTemp)
      lowTemp = temperature[i];
    else if (temperature[i] > highTemp)
      highTemp = temperature[i];
      
    if (humidity[i] < lowHum)
      lowHum = humidity[i];
    else if (humidity[i] > highHum)
      highHum = humidity[i];
  }

  avgTemp = avgTemp / 3;
  avgHum = avgHum / 3;

  if (avgTemp >= 10 && freezing = 0) {
    fanState = 1;
    digitalWrite(fanOutput, LOW);
  } else {
    fanState = 0;
    digitalWrite(fanOutput, HIGH);
  }
  
  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");

            if (freezing)
              client.println("<body style='background-color:red;'><h1>Mining Boy Climate Control</h1>");
            else
              client.println("<body style='background-color:white;'><h1>Mining Boy Climate Control</h1>");
              
            client.println("<p>MOBO Temp. : " + String(temperature[0]) + " // GPU Temp. : " + String(temperature[1]) + " // PSU Temp. : " + String(temperature[2]) + "</p>");
            client.println("<p>MOBO Hum. : " + String(humidity[0]) + " // GPU Hum. : " + String(humidity[1]) + " // PSU Hum. : " + String(humidity[2]) + "</p>");
            client.println("<p>Low Temp. : " + String(lowTemp) + " // Avg Temp. : " + String(avgTemp) + " // High Temp. : " + String(highTemp) + "</p>");
            client.println("<p>Low Hum. : " + String(lowHum) + " // Avg Hum. : " + String(avgHum) + " // High Hum. : " + String(highHum) + "</p>");
            
            if (fanState)
              client.println("<p>Fan State : On</p>");
            else
              client.println("<p>Fan State : Off</p>");

            if (freezing)
              client.println("<p>Freezing : YES</p>");
            else
              client.println("<p>Freezing : NO</p>");
            
            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
  }
}
