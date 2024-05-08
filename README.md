# M5Cardputer Chat with Gemini API


![Screenshot 2024-05-08 at 1 50 31 PM](https://github.com/vanshksingh/M5Cardputer-Chat-with-Gemini-API/assets/114809624/e70d0950-4690-4fcb-b4b4-d8838bcdc4f1)

An application that uses M5Cardputer to interact with the Gemini API for generating responses based on user input. This project includes code for managing Wi-Fi connectivity, handling user input, making API requests, and displaying responses on the M5Cardputer's screen.

## Features

- **User Input Handling**: Accepts user input through the M5Cardputer keyboard.
- **API Integration**: Sends queries to the Gemini API and processes the responses.
- **Display Management**: Manages the display of user input and responses.
- **Wi-Fi Connectivity**: Connects to the specified Wi-Fi network for API communication.
- **Configuration**: Reads configuration data such as Wi-Fi credentials and API key from an SD card.

## Installation

To use this project, follow the steps below:

1. Clone this repository:

    ```shell
    git clone https://github.com/vanshksingh/m5cardputer-chat-gemini-api.git
    ```

2. Navigate to the cloned repository:

    ```shell
    cd m5cardputer-chat-gemini-api
    ```

3. Open the project in your preferred IDE or text editor.

4. Upload the project to your M5Cardputer device.

5. Make sure you have the necessary libraries installed (e.g., M5GFX, WiFi, HTTPClient, ArduinoJson, SD).

6. Prepare a configuration file (`config.txt`) on an SD card with your Wi-Fi SSID, password, and API key, and insert the card into your M5Cardputer.

## Usage

1. Once the application is running on your M5Cardputer, follow the on-screen instructions to begin using the chat functionality.

2. Type your queries using the M5Cardputer keyboard, and the application will send them to the Gemini API.

3. The responses from the Gemini API will be displayed on the M5Cardputer screen.

## Contributing

Contributions are welcome! If you'd like to contribute to this project, please fork the repository and create a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

- **@vanshksingh**: [GitHub](https://github.com/vanshksingh)

Feel free to adjust the README to better suit your project and needs.
