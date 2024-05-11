#include "M5Cardputer.h"
#include "M5GFX.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include "SD.h"
#include "FS.h"
#include "SPI.h"





//images
#include "remixed.h"
#include "sd_img.h"
#include "wifi_img.h"
#include "generate.h"
#include "error.h"
#include "bg.h"

// Define maximum lines and line length
constexpr size_t maxLines = 100;
constexpr size_t maxLineLength = 100;

// Initialize lines array to hold parsed lines of text
char lines[maxLines][maxLineLength];
size_t numLines = 0;         // Number of parsed lines
size_t numDisplayableLines;  // Number of lines that can be displayed on the screen

// Canvas for drawing on the screen
M5Canvas canvas(&M5Cardputer.Display);

// Global variables
int scrollPosition = 0;
int scrollPositionDOWN = 0;
String userInput = "Double-check how you're handling spaces between words. Ensure that the width of spaces is correctly accounted for when calculating whether adding a word would exceed the display width.";                                                                                                                                                                                                                                    // Initial prompt
String geminiResponse = "Consider if there are any boundary conditions or edge cases where the logic might not work as expected. For example, if a word itself is wider than the display width, it might not be handled correctly by the current logic. By carefully reviewing each of these aspects, you should be able to identify and address the underlying cause of why some words are still going off-screen despite the logic provided.";  // Response from Gemini model
int retryAttempts = 5;                                                                                                                                                                                                                                                                                                                                                                                                                            // Number of retry attempts on request failure
String inputLines[maxLines];                                                                                                                                                                                                                                                                                                                                                                                                                      // Array to hold each line of input
int currentLineIndex = 0;                                                                                                                                                                                                                                                                                                                                                                                                                         // Index of the current line in the array


// Line height
int lineHeight;

// Function to parse text from String into lines
void parseText(const String &text) {
  numLines = 0;

  // Get the display width
  int displayWidth = M5Cardputer.Display.width();

  // Split input text into words and parse into lines
  String word;
  size_t currentLine = 0;
  char line[maxLineLength] = "";
  int currentLineWidth = 0;

  // Iterate through each character in the input text
  for (size_t i = 0; i < text.length(); i++) {
    char ch = text.charAt(i);

    if (ch == ' ' || i == text.length() - 1) {
      // If space or end of string, process the current word
      if (i == text.length() - 1 && ch != ' ') {
        // Append the last character if not a space
        word += ch;
      }

      // Calculate word width
      int wordWidth = canvas.textWidth(word.c_str(), nullptr) + 7;  //+6 for font offset

      // Check if adding the word would exceed display width
      if (currentLineWidth + wordWidth > displayWidth) {
        // Save the line and reset
        strncpy(lines[currentLine], line, maxLineLength);
        lines[currentLine][maxLineLength - 1] = '\0';
        currentLine++;
        line[0] = '\0';
        currentLineWidth = 0;
      }

      // Append word and a space to the line
      strncat(line, word.c_str(), maxLineLength);
      strncat(line, " ", 2);
      currentLineWidth += wordWidth;

      // Reset the word
      word = "";
    } else {
      // Add the current character to the word
      word += ch;
    }
  }

  // Save the last line if not exceeded maxLines
  if (currentLine < maxLines) {
    strncpy(lines[currentLine], line, maxLineLength);
    lines[currentLine][maxLineLength - 1] = '\0';
    currentLine++;
  }

  // Update numLines with the number of lines parsed
  numLines = currentLine;
}

void displayText(const String &text, int &scrollPosition, int scrollDirection, bool isSecondInput) {
  // Initialize M5Cardputer
  M5Cardputer.begin();

  // Display configuration
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(TC_DATUM);
  M5Cardputer.Display.setTextSize(2);

  // Create a canvas with the width of the display and the height of the text area
  int textAreaHeight = (M5Cardputer.Display.height() / 3) + 3;  // Use bottom 1/3 of the screen
  int textAreaStartY = M5Cardputer.Display.height() - textAreaHeight;

  if (isSecondInput) {

    // Draw a rectangle to mask the bottom 1/3 of the screen
    //M5Cardputer.Display.fillRect(0, textAreaStartY, M5Cardputer.Display.width(), textAreaHeight, GREEN);
    // Optionally, fill the area with a color (e.g., GREEN)
    M5Cardputer.Display.fillRect(0, textAreaStartY, M5Cardputer.Display.width(), textAreaHeight, BLUE);

    // Create a canvas for the second text
    canvas.createSprite(M5Cardputer.Display.width(), textAreaHeight);
    canvas.setColorDepth(8);
    canvas.setTextWrap(false);
    canvas.setTextSize(1.5);
    canvas.setTextColor(TFT_GREENYELLOW);
    //canvas.setFont(&Font2);


    // Calculate line height
    lineHeight = canvas.fontHeight();

    // Calculate the number of displayable lines for the bottom 1/3 of the screen
    numDisplayableLines = textAreaHeight / lineHeight;

    // Parse text into lines
    parseText(text);

    // Update scroll position based on scroll direction
    if (scrollDirection == -1) {
      // Scroll up
      if (scrollPositionDOWN > 0) {
        scrollPositionDOWN--;
      }
    } else if (scrollDirection == +1) {
      // Scroll down
      if (scrollPositionDOWN + numDisplayableLines < numLines) {
        scrollPositionDOWN++;
      }
    }

    // Clear the canvas
    canvas.fillScreen(TFT_NAVY);



    // Display lines on the canvas
    int y = 0;
    for (size_t i = scrollPositionDOWN; i < scrollPositionDOWN + numDisplayableLines && i < numLines; i++) {
      // Set cursor for each line
      canvas.setCursor(0, y);
      // Print line from the array
      canvas.print(lines[i]);
      // Increment y by line height for the next line
      y += lineHeight;
    }

    // Push the canvas content to the display, offsetting the starting position to the bottom 1/3 of the screen
    canvas.pushSprite(0, textAreaStartY);
    //M5Cardputer.Display.drawRect(0, textAreaStartY, M5Cardputer.Display.width(), textAreaHeight, BLUE);
  } else {
    // Display the first text normally, using the whole screen
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // Create a canvas for the first text
    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setColorDepth(8);
    canvas.setTextWrap(false);
    canvas.setTextSize(1.5);

    // Calculate line height
    lineHeight = canvas.fontHeight();

    // Calculate the number of displayable lines for the whole screen
    numDisplayableLines = M5Cardputer.Display.height() / lineHeight;

    // Parse text into lines
    parseText(text);

    // Update scroll position based on scroll direction
    if (scrollDirection == -1) {
      // Scroll up
      if (scrollPosition > 0) {
        scrollPosition--;
      }
    } else if (scrollDirection == +1) {
      // Scroll down
      if (scrollPosition + numDisplayableLines < numLines) {
        scrollPosition++;
      }
    }

    // Clear the canvas
    //canvas.fillScreen(TFT_DARKGREY);
    canvas.setTextColor(TFT_LIGHTGREY);

    // Display lines on the canvas
    int y = 0;
    for (size_t i = scrollPosition; i < scrollPosition + numDisplayableLines && i < numLines; i++) {
      // Set cursor for each line
      canvas.setCursor(0, y);
      // Print line from the array
      canvas.print(lines[i]);
      // Increment y by line height for the next line
      y += lineHeight;
    }

    // Push the canvas content to the display
    canvas.pushSprite(0, 0);
  }

  // Add a delay for smooth scrolling
  delay(20);
}

// Define SD card pins and SPI frequency
#define SD_SCK GPIO_NUM_40
#define SD_MISO GPIO_NUM_39
#define SD_MOSI GPIO_NUM_14
#define SD_SS GPIO_NUM_12
#define SD_SPI_FREQ 1000000

// Create an instance of the SPIClass for the HSPI
SPIClass hspi = SPIClass(HSPI);

//To run menu or not
bool Menu = false;

// Global variables

//String userInput = ""; // Store user input
int selectedOption = 0;    // Index of the currently selected menu option
const int numOptions = 5;  // Total number of menu options

// Menu options
const String menuOptions[numOptions] = {
  "1. Notes",
  "2. Read from SD Card",
  "3. Update Configuration",
  "4. Clear SD Card",
  "5. Exit Menu"
};

// Configuration variables
String ssid;
String password;
String api_key;

// Function to initialize the SD card
bool initializeSDCard() {
  // Begin the SPI interface
  hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_SS);

  // Initialize the SD card
  if (!SD.begin(SD_SS, hspi, SD_SPI_FREQ)) {
    printDebug("SD Card Initialization Failed!");
    return false;
  } else {
    printDebug("SD Card Initialized Successfully!");
    return true;
  }
}
// Function to display the saved configuration parameters on the screen
void displayConfig() {
  // Clear the display
  M5Cardputer.Display.fillScreen(BLACK);

  // Set the cursor position to the top-left corner
  M5Cardputer.Display.setCursor(0, 0);

  // Set the text color to white for visibility
  M5Cardputer.Display.setTextColor(WHITE);

  // Display the current configuration parameters
  M5Cardputer.Display.println("Configuration:");
  M5Cardputer.Display.println("SSID: " + ssid);
  M5Cardputer.Display.println("Password: " + password);
  M5Cardputer.Display.println("API Key: " + api_key);

  // Add a delay so the user can see the information
  delay(5000);
}

// Function to load configuration parameters from the SD card
void loadConfig() {
  // Define the configuration file name
  const String configFileName = "/config.txt";

  // Open the configuration file for reading
  File configFile = SD.open(configFileName, FILE_READ);

  // Check if the file is open
  if (configFile) {
    // Initialize temporary variables to hold the configuration values
    String line;

    // Read each line from the configuration file
    while (configFile.available()) {
      line = configFile.readStringUntil('\n');

      // Split the line into key-value pairs using the "=" delimiter
      int delimiterIndex = line.indexOf('=');
      if (delimiterIndex != -1) {
        String key = line.substring(0, delimiterIndex);
        String value = line.substring(delimiterIndex + 1);

        // Trim whitespace from the key and value
        key.trim();
        value.trim();

        // Update the global variables based on the key
        if (key == "SSID") {
          ssid = value;
        } else if (key == "PASSWORD") {
          password = value;
        } else if (key == "API_KEY") {
          api_key = value;
        }
      }
    }

    // Close the configuration file
    configFile.close();

    printDebug("Configuration loaded successfully.");
  } else {
    printDebug("Failed to open config file for reading.");
  }
}


void handleMenuSelection() {
  // Update the M5Cardputer state
  M5Cardputer.update();
  M5Cardputer.Display.setTextSize(0.8);
  M5Cardputer.Display.setTextFont(&fonts::FreeMono9pt7b);

  // Check if there is a change in the keyboard state
  if (M5Cardputer.Keyboard.isChange()) {
    // Check if a key is pressed
    if (M5Cardputer.Keyboard.isPressed()) {
      // Get the keyboard state
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      // Process each key in the status
      for (auto key : status.word) {
        // Move the selection up or down based on user input
        if (status.fn and key == ';') {
          // Move selection up
          selectedOption--;
          if (selectedOption < 0) {
            selectedOption = numOptions - 1;
          }
        } else if (status.fn and key == '.') {
          // Move selection down
          selectedOption++;
          if (selectedOption >= numOptions) {
            selectedOption = 0;
          }
        }
      }

      // Check if the Enter key is pressed
      if (status.enter) {
        M5Cardputer.Display.fillScreen(BLACK);
        // Perform the action associated with the selected option
        performMenuAction(selectedOption);
      }
    }
  }
}


// Function to handle user input
void handleUserInput() {
  // Reset user input
  userInput = "";

  // Display the input screen once
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.println("Enter your input:");
  M5Cardputer.Display.println(userInput);

  // Continuously process user input until Enter key is pressed
  while (true) {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange()) {
      // Check if a key is pressed
      if (M5Cardputer.Keyboard.isPressed()) {
        // Get the keyboard state
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // Process each key
        bool updateDisplay = false;
        for (auto key : status.word) {
          // Add the key to userInput
          userInput += key;
          updateDisplay = true;
        }

        // Handle enter key
        if (status.enter) {
          printDebug("You entered: " + userInput);
          return;
        }

        // Handle backspace key
        if (status.del && userInput.length() > 0) {
          userInput.remove(userInput.length() - 1);
          updateDisplay = true;
        }

        // Update the display only if there is a change in user input
        if (updateDisplay) {
          M5Cardputer.Display.fillRect(0, 20, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 20, BLACK);
          M5Cardputer.Display.setCursor(0, 20);
          M5Cardputer.Display.setTextColor(WHITE);
          M5Cardputer.Display.println(userInput);
        }
      }
    }
  }
}

// Function to save data to the SD card
void saveToSDCard(const String &data) {
  // Define the file name to save the data
  const String fileName = "/data.txt";

  // Open the file in append mode
  File file = SD.open(fileName, FILE_APPEND);

  // Check if the file is open
  if (file) {
    // Write the data to the file
    file.println(data);

    // Close the file
    file.close();

    printDebug("Data saved to SD card: " + data);
  } else {
    printDebug("Failed to open file for writing.");
  }
}

// Function to read data from the SD card
void readFromSDCard() {
  // Define the file name to read the data from
  const String fileName = "/data.txt";

  // Open the file in read mode
  File file = SD.open(fileName, FILE_READ);

  // Check if the file is open
  if (file) {
    printDebug("Reading data from SD card:");

    // Clear the canvas for data display
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(WHITE);

    // Read data line by line
    while (file.available()) {
      String line = file.readStringUntil('\n');
      M5Cardputer.Display.println(line);
    }

    // Push the display to the screen

    // Close the file
    file.close();
  } else {
    printDebug("Failed to open file for reading.");
  }
  delay(2000);
}

// Function to update configuration parameters
void updateConfig() {
  // Clear the screen
  M5Cardputer.Display.fillScreen(BLACK);

  // Variables for user input
  String newSSID;
  String newPassword;
  String newAPIKey;

  // Prompt the user to update SSID
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.println("Update SSID:");

  newSSID = handleUserInputAndGetResult();

  // Prompt the user to update Password
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.println("Update Password:");

  newPassword = handleUserInputAndGetResult();

  // Prompt the user to update API Key
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.println("Update API Key:");

  newAPIKey = handleUserInputAndGetResult();

  // Update the configuration values
  ssid = newSSID;
  password = newPassword;
  api_key = newAPIKey;

  // Save the updated configuration to the SD card
  saveConfig();
}

// Function to handle user input and get the result
String handleUserInputAndGetResult() {
  String inputResult = "";

  // Display the input screen once
  //M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.println("Enter your input:");
  M5Cardputer.Display.println(inputResult);

  // Continuously process user input until Enter key is pressed
  while (true) {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange()) {
      // Check if a key is pressed
      if (M5Cardputer.Keyboard.isPressed()) {
        // Get the keyboard state
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // Process each key
        bool updateDisplay = false;
        for (auto key : status.word) {
          // Add the key to the inputResult
          inputResult += key;
          updateDisplay = true;
        }

        // Handle enter key
        if (status.enter) {
          return inputResult;
        }

        // Handle backspace key
        if (status.del && inputResult.length() > 0) {
          inputResult.remove(inputResult.length() - 1);
          updateDisplay = true;
        }

        // Update the display only if there is a change in user input
        if (updateDisplay) {
          M5Cardputer.Display.fillRect(0, 20, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 20, BLACK);
          M5Cardputer.Display.setCursor(0, 20);
          M5Cardputer.Display.setTextColor(WHITE);
          M5Cardputer.Display.println(inputResult);
        }
      }
    }
  }
}

// Function to clear the SD card by deleting the data file
void clearSDCard() {
  // Define the file name to be cleared
  const String fileName = "/data.txt";

  // Delete the data file from the SD card
  if (SD.remove(fileName)) {
    // Display a success message if the file was deleted
    printDebug("SD card cleared: " + fileName + " deleted.");
  } else {
    // Display an error message if the file could not be deleted
    printDebug("Failed to clear SD card: " + fileName + " not found.");
  }
}

// Function to display the menu on the screen
void displayMenu() {
  if (Menu == true) {
    // Clear the screen
    //M5Cardputer.Display.fillScreen(BLACK);

    // Set cursor position and text color
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(WHITE);

    // Display each menu option
    for (int i = 0; i < numOptions; i++) {
      if (i == selectedOption) {
        // Highlight the currently selected option
        M5Cardputer.Display.setTextColor(RED);
      } else {
        // Display other options in white
        M5Cardputer.Display.setTextColor(WHITE);
      }
      M5Cardputer.Display.println(menuOptions[i]);
    }
  }
}

// Function to perform the action associated with the selected menu option
void performMenuAction(int option) {
  // After performing the action, display the menu again
  displayMenu();
  // Handle the selected option
  switch (option) {
    case 0:  // Option 1: Enter Input
      handleUserInput();
      if (userInput.isEmpty()) {
        printDebug("No input to save.");
      } else {
        saveToSDCard(userInput);
        userInput = "";
      }
      break;
    case 1:  // Option 2: Read From SD Card
      readFromSDCard();
      displayConfig();
      break;
    case 2:  // Option 3: Update Config
      updateConfig();
      break;
    case 3:  // Option 4: Clear SD Card
      clearSDCard();
      break;
    case 4:  // Option 5:
      printDebug("Exiting menu...");
      Menu = false;
      M5Cardputer.Display.fillScreen(BLACK);
      break;
    default:
      printDebug("Invalid option selected.");
      break;
  }
  M5Cardputer.Display.fillScreen(BLACK);
}

// Function to print debug output on the screen
void printDebug(const String &message) {
  // Clear the canvas screen to display the message clearly
  M5Cardputer.Display.fillScreen(BLACK);

  // Set the cursor position to the top-left corner
  M5Cardputer.Display.setCursor(0, 0);

  // Set the text color to white for visibility
  M5Cardputer.Display.setTextColor(WHITE);

  // Print the provided debug message
  M5Cardputer.Display.println(message);

  // Delay for visibility
  delay(2000);
}

// Function to save configuration parameters to the SD card
void saveConfig() {
  // Define the configuration file name
  const String configFileName = "/config.txt";

  // Open the configuration file for writing
  File configFile = SD.open(configFileName, FILE_WRITE);

  // Check if the file is open
  if (configFile) {
    // Write the configuration parameters to the file
    configFile.println("SSID=" + ssid);
    configFile.println("PASSWORD=" + password);
    configFile.println("API_KEY=" + api_key);

    // Close the configuration file
    configFile.close();

    printDebug("Configuration updated successfully.");
  } else {
    printDebug("Failed to open config file for writing.");
  }
}

// Function to connect to Wi-Fi
void connectToWiFi() {

  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.println("Connecting to Wi-Fi...");

  WiFi.begin(ssid.c_str(), password.c_str());
  int attempts = 0;
  const int maxAttempts = 10;

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5Cardputer.Display.print(".");
    attempts++;

    if (attempts >= maxAttempts) {

      M5Cardputer.Display.setTextColor(RED);

      M5Cardputer.Display.println("\nFailed to connect to Wi-Fi, switching to menu...");
      delay(2000);
      M5Cardputer.Display.fillScreen(BLACK);
      Menu = true;
      ConfigMenu();
      M5Cardputer.Display.fillScreen(BLACK);
      M5Cardputer.Display.setTextColor(RED);
      M5Cardputer.Display.println("\nRestarting...");
      delay(2000);
      ESP.restart();
    }
  }

  M5Cardputer.Display.println("\nWi-Fi connected");
  M5Cardputer.Display.println("\nIP: " + WiFi.localIP().toString());
  delay(1000);
  M5Cardputer.Display.fillScreen(BLACK);
}


void setup() {
  // Initialize the M5Cardputer
  M5Cardputer.begin();
  M5Cardputer.Display.setSwapBytes(true);
  // Set display rotation and text size
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.pushImage(0, 0, 240, 135, remixed); // Render the image at the origin (0, 0)
    delay(2000);
  M5Cardputer.Display.setTextSize(0.9);
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextFont(&fonts::FreeMono9pt7b);

  M5Cardputer.Display.pushImage(0, 0, 240, 135, sd_img);
  delay(2000);
  // Initialize the SD card
  if (initializeSDCard()) {
    printDebug("Setup completed.");
    loadConfig();
  } else {
    printDebug("Setup failed.");
    M5Cardputer.Display.fillScreen(BLACK);
    Menu = true;
    ConfigMenu();
  }
  M5Cardputer.Display.pushImage(0, 0, 240, 135, wifi_img);
  delay(2000);
  connectToWiFi();
  M5Cardputer.Display.fillScreen(BLACK);
  displayText("> ", scrollPositionDOWN, 0, true);

  // Display the initial menu
}

void ConfigMenu() {
  while (Menu == true) {
    // Handle menu selection
    handleMenuSelection();

    // Display the menu
    displayMenu();
  }
}


// Function to handle backspace and update user input display
void handleBackspace() {

  // Check if there is text to delete
  if (userInput.length() > 0) {  // "> " length is 2
    parseText(userInput);
    int BeforeDelete = numLines;
    // Remove the last character from userInput
    userInput.remove(userInput.length() - 1);  
    parseText(userInput);
    int AfterDelete = numLines;
    if (BeforeDelete == AfterDelete) {
      displayText(userInput, scrollPositionDOWN, 0, true);
    } else {
      displayText(userInput, scrollPositionDOWN, -1, true);
    }
  }
}



void loop() {
  if (Menu == true) {
    ConfigMenu();
  }
  // Update the M5Cardputer state
  M5Cardputer.update();

  // Handle user input from the keyboard
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {

      // Get the keyboard state
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      // Process each key in the status
      for (auto key : status.word) {
        if (!(status.fn or status.ctrl)) { //somehow status.ctrl is not working in the loop , but is working outside ?
          userInput += key;
          displayText(userInput, scrollPositionDOWN, +1, true);  // Only append key if neither fn nor ctrl is pressed
        }

        if (status.fn and key == ';') {
          displayText(geminiResponse, scrollPosition, -1, false);
        }

        if (status.fn and key == '.') {
          
          displayText(geminiResponse, scrollPosition, +1, false);

        if (status.ctrl and key == ',' ) {
        
        displayText(userInput, scrollPositionDOWN, -1, true);
      }

      if (status.ctrl and key == '/') {
        
        displayText(userInput, scrollPositionDOWN, +1, true);
      }
        }
      }


      
      // Handle backspace key
      if (status.del) {
        handleBackspace();
      }

      if (status.opt) {
        Menu = true;
        M5Cardputer.Display.fillScreen(BLACK);
      }

      // Handle enter key (submit input)
      if (status.enter) {

        // Remove the initial prompt from the input
        //String inputWithoutPrompt = userInput.substring(2);
        String inputWithoutPrompt = userInput;

        // Clear the display to prepare for the response
        M5Cardputer.Display.setTextSize(1.2);
        M5Cardputer.Display.setTextFont(&fonts::Orbitron_Light_24);

        // Clear the display
M5Cardputer.Display.pushImage(0, 0, 240, 135, generate);
        

        // Send a request to the Gemini API with the user input (without prompt)
        if (!sendGeminiRequest(inputWithoutPrompt)) {
          // If the request fails, retry up to the specified number of times
          for (int i = 0; i < retryAttempts; i++) {
            if (sendGeminiRequest(inputWithoutPrompt)) {
              break;
            }
          }
        }

        // Reset the user input prompt after handling the request
        userInput = "";
        scrollPosition = 0;
        scrollPositionDOWN = 0;
      }
    }
  }
}


// Function to send a request to the Gemini API
bool sendGeminiRequest(const String &input) {
  // Construct the Gemini API URL with the API key
  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=" + String(api_key);

  // Create an HTTP client
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Create the JSON request body
  DynamicJsonDocument requestBody(1024);

  // Add prefix "Keep Responses very short and to the point" to the initial input
  static bool isFirstRequest = true;
  String requestText = input;
  if (isFirstRequest) {
    requestText = "Keep Responses very short and to the point: " + requestText;
    isFirstRequest = false;  // After the first request, do not prefix anymore
  }

  // Fill the request body with the user's request text
  requestBody["contents"][0]["role"] = "user";
  requestBody["contents"][0]["parts"][0]["text"] = requestText;

  // Convert JSON document to a string
  String requestBodyStr;
  serializeJson(requestBody, requestBodyStr);

  // Send HTTP POST request
  int httpResponseCode = http.POST(requestBodyStr);
  if (httpResponseCode == HTTP_CODE_OK) {
    // Get the response
    String response = http.getString();

    // Process the Gemini response
    processGeminiResponse(response);

    // End the HTTP connection
    http.end();
    return true;  // Request succeeded
  } else {
    // Display an HTTP error
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("\nHTTP error: " + String(httpResponseCode));
    M5Cardputer.Display.println("Error: " + http.errorToString(httpResponseCode));


    // End the HTTP connection
    http.end();
    return false;  // Request failed
  }
}

// Function to process the Gemini response
void processGeminiResponse(const String &response) {
  // Create a JSON document to hold the response
  DynamicJsonDocument responseDoc(2048);
  deserializeJson(responseDoc, response);

  // Extract the Gemini API response
  geminiResponse = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();

  M5Cardputer.Display.fillScreen(BLACK);
  // Display the Gemini API response at the top of the screen
  displayText(geminiResponse, scrollPosition, 0, false);
}
