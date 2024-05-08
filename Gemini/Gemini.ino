


/*
    M5Cardputer Chat with Gemini API
    
    @date: 2024-05-08
    @time: 1:34 pm
    @author: @vanshksingh
    @github: https://github.com/vanshksingh/M5Cardputer-Chat-with-Gemini-API/
    Description:
    This code enables an M5Cardputer device to interact with the Gemini API
    for generating responses based on user input. The user inputs queries
    through the device, and the program sends these queries to the Gemini API
    and displays the responses received from the API. The code manages the
    display, handles user input and API requests, and includes functionality
    for connecting to Wi-Fi and reading configuration data from an SD card.
    
    Key Components:
    - M5Cardputer: An ESP32-based development board that includes a display, keyboard,
      and other components.
    - WiFi: Used for connecting the device to the internet to communicate with the Gemini API.
    - HTTPClient: Used for sending HTTP requests to the Gemini API and receiving responses.
    - ArduinoJson: Library for parsing and creating JSON data.
    - SD: Library for interacting with an SD card for reading configuration data.
    - FS: Filesystem library for managing the SD card.
    - SPI: Serial Peripheral Interface library for initializing the SD card interface.
    
    Key Global Variables:
    - userInput: Holds the current user input including the prompt.
    - geminiResponse: Holds the response received from the Gemini API.
    - retryAttempts: Number of retry attempts allowed for a failed API request.
    - scrollingEnabled: Boolean flag indicating whether scrolling is enabled for long responses.
    - inputLines: Array holding the history of user inputs for display.
    - currentLineIndex: Index indicating the current position in the input history.
    - ssid, password, api_key: Wi-Fi credentials and API key loaded from the config file.
    
    Functions:
    - setup(): Initializes the M5Cardputer, connects to Wi-Fi, and loads configuration data from an SD card.
    - loadConfig(): Reads and parses the configuration file from the SD card.
    - connectToWiFi(): Connects the device to the specified Wi-Fi network.
    - displayUserInput(): Displays user input on the screen, handling line wrapping and scrolling.
    - handleBackspace(): Handles backspace key input and updates the displayed user input.
    - displayGeminiResponse(): Displays the response from the Gemini API on the screen.
    - sendGeminiRequest(const String& input): Sends a request to the Gemini API with the user's input and handles the response.
    - processGeminiResponse(const String& response): Parses the response from the Gemini API and displays it on the screen.
    
    In the main loop, the program continuously checks for keyboard input and handles different
    key actions such as adding to the user input, backspacing, and submitting the user input
    to the Gemini API. The response from the API is processed and displayed to the user.
*/



#include "M5Cardputer.h"
#include "M5GFX.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include "SD.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <FastLED.h>


#define SD_SCK  GPIO_NUM_40
#define SD_MISO GPIO_NUM_39
#define SD_MOSI GPIO_NUM_14
#define SD_SS   GPIO_NUM_12
#define SD_SPI_FREQ 1000000

#define PIN_LED    21
    #define NUM_LEDS   1
 CRGB leds[NUM_LEDS];
    uint8_t led_ih             = 0;
    uint8_t led_status         = 0;
    

SPIClass hspi = SPIClass(HSPI);

// Global variables
M5Canvas canvas(&M5Cardputer.Display);
String userInput = "> "; // Initial prompt
String geminiResponse = ""; // Response from Gemini model
int retryAttempts = 5; // Number of retry attempts on request failure
bool scrollingEnabled = false; // Flag for enabling scrolling
const int maxLines = 10; // Maximum number of lines to display
String inputLines[maxLines]; // Array to hold each line of input
int currentLineIndex = 0; // Index of the current line in the array
String ssid, password, api_key; // Variables for SSID, password, and API key

// Setup function
void setup() {
    // Initialize M5Cardputer
    M5Cardputer.begin();

    FastLED.addLeds<WS2812, PIN_LED, GRB>(leds, NUM_LEDS);

    leds[0] = CRGB::Cyan;
    FastLED.show();

    // Set display rotation and text size
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextSize(0.1);
    M5Cardputer.Display.println("/n ");
    M5Cardputer.Display.setTextSize(1.4);
    M5Cardputer.Display.setTextFont(&fonts::Orbitron_Light_24);

    // Clear the display
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.println("      Gemini");
    M5Cardputer.Display.setTextSize(0.7);
    M5Cardputer.Display.setTextFont(&fonts::Yellowtail_32);
    M5Cardputer.Display.setTextColor(ORANGE);
    M5Cardputer.Display.println("                 Remixed!");
    M5Cardputer.Display.setTextSize(0.5);
    M5Cardputer.Display.setTextFont(&fonts::Yellowtail_32);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("                     @vanshksingh");

    delay(2000);
    M5Cardputer.Display.setTextSize(0.9);
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextFont(&fonts::FreeMono9pt7b);

    hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_SS);

    // Initialize the SD card using the specified chip select pin
    if (!SD.begin(SD_SS, hspi, SD_SPI_FREQ)) {
      leds[0] = CRGB::Red;
    FastLED.show();
        M5Cardputer.Display.setTextColor(RED);
        M5Cardputer.Display.setCursor(2, 0);
        M5Cardputer.Display.println(" SD Card Init Failed!");
        delay(2000);
        ESP.restart();
    } else {
      leds[0] = CRGB::Gold;
    FastLED.show();
        M5Cardputer.Display.setTextColor(YELLOW);
        M5Cardputer.Display.setCursor(2, 0);
        M5Cardputer.Display.println(" SD Card Initialized!");
        delay(1000);

        // Load the configuration from the config file on the SD card
        if (loadConfig()) {
            // Connect to Wi-Fi
            connectToWiFi();
        } else {
          leds[0] = CRGB::Violet;
    FastLED.show();
            M5Cardputer.Display.setTextColor(RED);
            M5Cardputer.Display.println("\nFailed to load config file.");
            delay(2000);
            ESP.restart();
        }
    }

    // Display the initial prompt for user input
    displayUserInput();
}


// Function to load configuration data from the config file on the SD card
bool loadConfig() {
    // Open the configuration file on the SD card
    File configFile = SD.open("/config.txt", FILE_READ);
    if (!configFile) {
        return false;
    }

    // Read the configuration file line by line
    while (configFile.available()) {
        // Read each line from the file
        String line = configFile.readStringUntil('\n');
        // Trim the line to remove any surrounding whitespace
        line.trim();

        // Parse the line to extract the parameter and value
        int delimiterIndex = line.indexOf('=');
        if (delimiterIndex != -1) {
            // Extract the parameter and value
            String parameter = line.substring(0, delimiterIndex);
            String value = line.substring(delimiterIndex + 1);

            // Trim the parameter and value separately
            parameter.trim();
            value.trim();

            // Assign the values to the appropriate variables
            if (parameter == "SSID") {
                ssid = value;
            } else if (parameter == "PASSWORD") {
                password = value;
            } else if (parameter == "API_KEY") {
                api_key = value;
            }
        }
    }

    // Close the configuration file
    configFile.close();
    return true;
}



// Function to connect to Wi-Fi
void connectToWiFi() {
  leds[0] = CRGB::Orange;
    FastLED.show();
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
            leds[0] = CRGB::Orange;
    FastLED.show();
            M5Cardputer.Display.println("\nFailed to connect to Wi-Fi, restarting...");
            delay(2000);
            ESP.restart();
        }
    }
leds[0] = CRGB::Green;
    FastLED.show();
    M5Cardputer.Display.println("\nWi-Fi connected");
    M5Cardputer.Display.println("\nIP: " + WiFi.localIP().toString());
    delay(1000);
    M5Cardputer.Display.fillScreen(BLACK);
}

// Continue with the rest of your program such as displayUserInput(), handleBackspace(), sendGeminiRequest(), processGeminiResponse(), and loop().


// Function to handle backspace and update user input display
void handleBackspace() {
    // Check if there is text to delete
    if (userInput.length() > 2) { // "> " length is 2
        // Remove the last character from userInput
        userInput.remove(userInput.length() - 1);
        M5Cardputer.Display.fillScreen(BLACK);

        // Display updated user input
        displayUserInput();
    }
}

// Function to handle user input and display by wrapping and pushing lines up
void displayUserInput() {
    // Calculate the height of one line of text
    int inputHeight = 16; // Adjust if necessary

    // Calculate the bottom line Y position of the display
    int bottomLineY = M5Cardputer.Display.height() - inputHeight;

    // Calculate the width of the user's input text
    int textWidth = M5Cardputer.Display.textWidth(userInput);

    // Check if the input text width exceeds the display width
    if (textWidth > M5Cardputer.Display.width()) {
        // If the array is full, push existing lines up
        if (currentLineIndex >= maxLines) {
            for (int i = 0; i < maxLines - 1; i++) {
                inputLines[i] = inputLines[i + 1];
            }
        } else {
            currentLineIndex++;
        }
        
        // Add the current user input to the last line of the array
        inputLines[currentLineIndex] = userInput;
        
        // Clear the display
        M5Cardputer.Display.fillScreen(BLACK);

        // Display all lines of input from the array
        for (int i = 0; i <= currentLineIndex; i++) {
            // Calculate the Y position for each line of text
            int lineY = bottomLineY - (inputHeight * i);

            // Set text color and cursor position
            M5Cardputer.Display.setTextColor(GREENYELLOW);
            M5Cardputer.Display.setCursor(0, lineY);

            // Print the line of input
            M5Cardputer.Display.print(inputLines[i]);
        }

        // Reset the user input to an initial prompt for new input
        userInput = "> ";
    } else {
        // If the input text width does not exceed the display width, display it on the bottom line
        M5Cardputer.Display.setTextColor(GREENYELLOW);
        M5Cardputer.Display.setCursor(0, bottomLineY);
        M5Cardputer.Display.print(userInput);
      leds[0] = CRGB::SkyBlue;
    FastLED.show();
    }
}

// Function to handle the response from the Gemini API
void displayGeminiResponse() {
    // Clear the line where the Gemini response is displayed
    M5Cardputer.Display.fillRect(0, 0, M5Cardputer.Display.width(), 16, BLACK);
    
    // Display the Gemini response in a distinct color
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(0, 0);
    
    // Enable scrolling if the response is too long
    if (geminiResponse.length() > M5Cardputer.Display.width()) {
        scrollingEnabled = true;
        canvas.setTextScroll(true);
        canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height() - 36);
        canvas.fillScreen(BLACK);
        canvas.setTextColor(WHITE);
        canvas.println("Response: " + geminiResponse);
        canvas.pushSprite(0, 0);
        leds[0] = CRGB::White;
    FastLED.show();
    } else {
      leds[0] = CRGB::Gray;
    FastLED.show();
        M5Cardputer.Display.print("Response: " + geminiResponse);
        delay(1000);
    }
}

// Function to handle user input and API response
void loop() {
    // Update the M5Cardputer state
    M5Cardputer.update();

    // Handle user input from the keyboard
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
          leds[0] = CRGB::SeaGreen;
    FastLED.show();
            // Get the keyboard state
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

            // Process each key in the status
            for (auto key : status.word) {
                userInput += key;
            }

            // Handle backspace key
            if (status.del) {
                handleBackspace();
                leds[0] = CRGB::IndianRed;
    FastLED.show();
            }

            // Handle enter key (submit input)
            if (status.enter) {
              leds[0] = CRGB::PaleTurquoise;
    FastLED.show();
                // Remove the initial prompt from the input
                String inputWithoutPrompt = userInput.substring(2);

                // Clear the display to prepare for the response
                M5Cardputer.Display.setTextSize(1.2);
    M5Cardputer.Display.setTextFont(&fonts::Orbitron_Light_24);

    // Clear the display
    leds[0] = CRGB::DarkBlue;
    FastLED.show();
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("Generating");
    M5Cardputer.Display.println("Response...");
    M5Cardputer.Display.setTextSize(0.9);
    M5Cardputer.Display.setTextFont(&fonts::FreeMono9pt7b);

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
                userInput = "> ";

                // Display the user input prompt
                displayUserInput();
            }

            // Display the current user input
            displayUserInput();
        }
    }
}

// Function to send a request to the Gemini API
bool sendGeminiRequest(const String& input) {
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
        isFirstRequest = false; // After the first request, do not prefix anymore
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
        return true; // Request succeeded
    } else {
        // Display an HTTP error
        M5Cardputer.Display.setTextColor(RED);
        M5Cardputer.Display.print("\nHTTP error: " + String(httpResponseCode));
        
        // End the HTTP connection
        http.end();
        return false; // Request failed
    }
}

// Function to process the Gemini response
void processGeminiResponse(const String& response) {
    // Create a JSON document to hold the response
    DynamicJsonDocument responseDoc(2048);
    deserializeJson(responseDoc, response);

    // Extract the Gemini API response
    geminiResponse = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();

    M5Cardputer.Display.fillScreen(BLACK);
    // Display the Gemini API response at the top of the screen
    displayGeminiResponse();
}
