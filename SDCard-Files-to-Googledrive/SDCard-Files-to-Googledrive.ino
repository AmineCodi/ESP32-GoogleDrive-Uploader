/* The ESP32 Google Drive Uploader repository contains an ESP32 project that enables users to upload text files from an SD card to Google Drive.
The code reads files from the SD card, connects to Wi-Fi, and sends them to a Google Apps Script for storage. 
It supports various text file formats (.txt, .csv, .log) while automatically skipping non-text files,
and establishes a secure HTTPS connection for uploads.
*/
const char* ssid     = "WIFI-ssid";  //customize Wi-Fi ssid
const char* password = "WIFI-password";  // customize Wi-Fi password

const char* myDomain = "script.google.com";
String myScript = "myScript-URL";  // Set Google Script path
String myFoldername = "myDriveFolder";  // Set Google Drive folder name to store text data

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
#include <esp_task_wdt.h>


#define SD_FAT_TYPE 0
const uint8_t SD_CS_PIN = 26;
const uint8_t SOFT_MISO_PIN = 13;
const uint8_t SOFT_MOSI_PIN = 14;
const uint8_t SOFT_SCK_PIN = 27;

SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)

SdFat sd;
File file;
File root;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(10);
  
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  
  
  long int StartTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if ((StartTime + 10000) < millis()) break;
  } 

  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed");
    ESP.restart();
  }

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  sendAllFilesToGoogleDrive();
}

void loop() {
  delay(100);
}

void sendAllFilesToGoogleDrive() {
  if (!root.open("/")) {
    Serial.println("Failed to open root directory");
    return;
  }

  while (file.openNext(&root, O_RDONLY)) {
    char filename[256];
    file.getName(filename, sizeof(filename));
    String filenameStr = String(filename);

    if (!file.isDir() && isASCIIFile(filenameStr)) {
      Serial.print("Sending file: ");
      Serial.println(filenameStr);
      
      // Read the file content
      String fileContent = readFile(filenameStr);
      Serial.print("Sending to Google Drive: ");
      Serial.println(filenameStr);
      delay(1000);
      
      if (fileContent == "") {
        Serial.println("Failed to read file or file is empty");
        continue; // Skip to the next file
      }

      String Data = "myFoldername=" + urlencode(myFoldername) + "&myFilename=" + urlencode(filenameStr) + "&myFile=" + urlencode(fileContent);  
  
      Serial.println("Connect to " + String(myDomain));
      WiFiClientSecure client_tcp;

      if (client_tcp.connect(myDomain, 443)) {
        Serial.println("Connection successful");

        String postRequest = "POST " + myScript + " HTTP/1.1\r\n" +
                             "Host: " + String(myDomain) + "\r\n" +
                             "Content-Type: application/x-www-form-urlencoded\r\n" +
                             "Content-Length: " + String(Data.length()) + "\r\n" +
                             "Connection: close\r\n\r\n" +
                             Data;

        client_tcp.print(postRequest);
        Serial.println("Request sent");
        
        unsigned long timeout = millis();
        while (client_tcp.available() == 0) {
          if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client_tcp.stop();
            break; // Skip to the next file
          }
        }
          
        String response = "";
        String redirectUrl = "";
        bool redirect = false;
        
        while(client_tcp.available()) {
          String line = client_tcp.readStringUntil('\n');
          if (line.startsWith("Location: ")) {
            redirectUrl = line.substring(10);
            redirectUrl.trim();
            redirect = true;
          }
          if (line == "\r") {
            break;
          }
          response += line + "\n";
        }
        
        client_tcp.stop();
        
        if (redirect) {
          Serial.println("Redirecting to: " + redirectUrl);
          delay(2000);
          sendRedirectRequest(redirectUrl, Data);
        }
        
        Serial.println("Response:");
        Serial.println(response);
        Serial.println("Connection closed");
      } else {
        Serial.println("Connection to " + String(myDomain) + " failed.");
      }
    }

    file.close();
  }

  root.close();
  Serial.println("All files were sent to Google Drive successfully.");
}

bool isASCIIFile(String filename) {
  filename.toLowerCase();
  return filename.endsWith(".txt") || filename.endsWith(".csv") || 
         filename.endsWith(".log") || filename.endsWith(".ini") || 
         filename.endsWith(".xml") || filename.endsWith(".json") ||
         filename.endsWith(".cfg") || filename.endsWith(".conf") ||
         filename.endsWith(".asc") || filename.endsWith(".dat");
}

String sendRedirectRequest(String url, String data) {
  WiFiClientSecure client_tcp;
  
  int colonIndex = url.indexOf("://");
  int slashIndex = url.indexOf('/', colonIndex + 3);
  String host = url.substring(colonIndex + 3, slashIndex);
  String path = url.substring(slashIndex);

  if (client_tcp.connect(host.c_str(), 443)) {
    String postRequest = "POST " + path + " HTTP/1.1\r\n" +
                         "Host: " + host + "\r\n" +
                         "Content-Type: application/x-www-form-urlencoded\r\n" +
                         "Content-Length: " + String(data.length()) + "\r\n" +
                         "Connection: close\r\n\r\n" +
                         data;

    client_tcp.print(postRequest);
    
    String response = "";
    while(client_tcp.available()) {
      char c = client_tcp.read();
      response += c;
    }
    
    client_tcp.stop();
    return response;
  } else {
    return "Redirect connection failed";
  }
}

String readFile(String filename) {
  File dataFile = sd.open(filename.c_str(), FILE_READ);
  if (!dataFile) {
    Serial.println("Failed to open file for reading");
    return "";
  }

  String content = "";
  while (dataFile.available()) {
    content += (char)dataFile.read();
  }
  dataFile.close();
  return content;
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}
