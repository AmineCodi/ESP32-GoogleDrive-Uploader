// Google Apps Script

/*This Google Apps Script handles POST requests for uploading text files to Google Drive. It processes incoming data,
checks for required parameters, and manages file creation or appending based on chunked uploads.
The script includes rate limiting to prevent excessive requests and logs errors for debugging.
It returns a JSON response indicating the success or failure of the upload operation, along with the file's URL if successfully uploaded. 
Ideal for integrating with IoT devices like ESP32 for cloud storage solutions.*/
function doPost(e) {
  var response = {
    status: "error",
    message: "Unknown error occurred"
  };

  try {
    checkRateLimit();
    
    console.log("Received event: " + JSON.stringify(e));

    if (!e || !e.postData) {
      throw new Error("No post data received");
    }

    console.log("Received data: " + e.postData.contents);

    var params = {};
    e.postData.contents.split('&').forEach(function(param) {
      var parts = param.split('=');
      params[parts[0]] = decodeURIComponent(parts[1].replace(/\+/g, ' '));
    });

    console.log("Parsed parameters: " + JSON.stringify(params));

    var myFoldername = params.myFoldername;
    var myFilename = params.myFilename;
    var fileContent = params.myFile;
    var chunkIndex = parseInt(params.chunkIndex || "0");
    var totalChunks = parseInt(params.totalChunks || "1");

    if (!myFoldername || !myFilename) {
      throw new Error("Missing required parameters");
    }

    var folder;
    var folders = DriveApp.getFoldersByName(myFoldername);
    if (folders.hasNext()) {
      folder = folders.next();
    } else {
      folder = DriveApp.createFolder(myFoldername);
    }

    var file;
    if (chunkIndex === 0) {
      file = folder.createFile(myFilename, fileContent, MimeType.PLAIN_TEXT);
    } else {
      file = folder.getFilesByName(myFilename).next();
      file.setContent(file.getBlob().getDataAsString() + fileContent);
    }

    if (chunkIndex === totalChunks - 1) {
      file.setDescription("Uploaded by ESP32");
      var fileId = file.getId();
      var fileUrl = "https://drive.google.com/uc?export=view&id=" + fileId;
      response.url = fileUrl;
    }

    response.status = "success";
    response.message = "File chunk uploaded successfully";
    response.filename = myFilename;
    response.folder = myFoldername;
    response.chunkIndex = chunkIndex;
    response.totalChunks = totalChunks;

  } catch (error) {
    response.message = "Error: " + error.toString();
    logError(error);
  }

  return ContentService.createTextOutput(JSON.stringify(response))
    .setMimeType(ContentService.MimeType.JSON);
}

function doGet(e) {
  return ContentService.createTextOutput("This script is designed to handle POST requests for text file uploads.");
}

function checkRateLimit() {
  var cache = CacheService.getScriptCache();
  var calls = parseInt(cache.get('api_calls') || "0");
  calls++;
  cache.put('api_calls', calls.toString(), 60); // Store for 60 seconds
  if (calls > 10) { // Limit to 10 calls per minute
    throw new Error("Rate limit exceeded");
  }
}

function logError(error) {
  console.error("Error: " + error.toString());
  Logger.log("ERROR: " + error.toString());
}
