# ESP32-GoogleDrive-Uploader
This ESP32 project allows easy file uploads from your microcontroller to Google Drive. It provides a straightforward way to send data, logs, or files to the cloud for storage or processing. The repo includes setup instructions, example code, and libraries to help you quickly integrate Google Drive file uploads into your IoT projects.
## Features
Easily upload text files to Google Drive from an ESP32.
Simple configuration for Wi-Fi credentials and Google Drive settings.
Customizable content to be sent in the uploaded file.
Secure connection using HTTPS.
## Getting Started
### Prerequisites
*  ESP32 development board
* Arduino IDE with ESP32 board support
* A Google account to set up Google Apps Script

### Setup Instructions
 1.  Clone the Repository:
'''bash
git clone https://github.com/yourusername/ESP32-GoogleDrive-Uploader.git
cd ESP32-GoogleDrive-Uploader'''
 2. Configure Wi-Fi Credentials:
Open the code file and update the ssid and password variables with your Wi-Fi network credentials:
'''cpp
const char* ssid = "WIFI";  // Wi-Fi account
const char* password = "AAAAAAAA";  // Wi-Fi password'''
 3. Set Google Apps Script:
Create a [Google Script](https://script.google.com/home "Google Script Home") Google Apps Script to handle file uploads. Use the provided URL in the myScript variable:
''' 
cpp
String myScript = "https://script.google.com/macros/s/your_script_id/exec";  // Set Google Script path'''   

 4. Customize File Content:
Edit the fileContent variable to change the content of the file you want to upload:
''' 
cpp
String fileContent = "This is the content of the file.";  // Edit this string to change the file content'''
 5. Upload the Code: 
 Connect your ESP32 board to your computer and upload the code using the Arduino IDE.

 6.  Monitor Output:
Open the Serial Monitor to view connection status and responses from the Google Apps Script.
## Code Overview


* The code connects to the specified Wi-Fi network.
* It constructs a POST request to send data to the Google Apps Script.
* The content of the file is URL-encoded before being sent.
* The response from the server is printed to the Serial Monitor.
## Contributing
Feel free to submit issues or pull requests if you have suggestions or improvements.
## License
This project is licensed under the MIT License. See the LICENSE file for more details.
## Acknowledgments
Thanks to the ESP32 community for their support and resources.
Special thanks to Google for providing the Apps Script platform for easy file uploads.
