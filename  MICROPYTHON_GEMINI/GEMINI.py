from machine import Pin, SPI
import st7789py as st7789
from font import vga2_8x8 as font  # Using the VGA 2 8x8 font
from keyboard_module import KeyBoard
import urequests
import ujson
import network
import time
import config

# Customization variables
# Line numbers for input and responses
LINE_START_INPUT = 10  # Starting line for user input
LINE_START_RESPONSE = 2  # Starting line for response

# Colors
COLOR_BACKGROUND = st7789.BLACK  # Background color
COLOR_INPUT = st7789.BLUE  # Color for user input
COLOR_RESPONSE = st7789.WHITE  # Color for responses
COLOR_ERROR = st7789.RED  # Color for error messages

# Messages
MSG_INPUT_PROMPT = "Model Ready for Input "
MSG_SENDING_INPUT = "Sending Input...      "
MSG_SERVER_ERROR = "Server Error 500"

# Define the display parameters and initialize the display
tft = st7789.ST7789(
    SPI(2, baudrate=40000000, sck=Pin(36), mosi=Pin(35), miso=None),
    135,  # Width
    240,  # Height
    reset=Pin(33, Pin.OUT),
    cs=Pin(37, Pin.OUT),
    dc=Pin(34, Pin.OUT),
    backlight=Pin(38, Pin.OUT),
    rotation=1,
    color_order=st7789.BGR
)

# Start with a black background
tft.fill(COLOR_BACKGROUND)

# Define your Wi-Fi network credentials
ssid = config.ssid
password = config.password

# Define the Google API key and URL
api_key = config.api_key
url = f'https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key={api_key}'

# Define the request headers
headers = {
    'Content-Type': 'application/json'
}

# Function to connect to the Wi-Fi network
def connect_to_wifi(ssid, password):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)
    
    # Retry connection a few times with a delay
    retries = 5
    while not wlan.isconnected() and retries > 0:
        print('Connecting to Wi-Fi...')
        time.sleep(5)
        retries -= 1
    
    if wlan.isconnected():
        print('Connected to Wi-Fi:', wlan.ifconfig())
        return True
    else:
        print('Failed to connect to Wi-Fi')
        return False

# Function to display text on the display
def display_text(line, text, color=COLOR_RESPONSE):
    # Calculate x, y coordinates based on the line number
    x = 0
    y = line * font.HEIGHT
    print(f'Displaying text: "{text}" on line {line}')
    tft.text(font, text, x, y, color)

# Function to clear a single character from the display at a specific x, y position
def clear_char(x, y):
    tft.fill_rect(x, y, font.WIDTH, font.HEIGHT, COLOR_BACKGROUND)

# Function to split long text into lines based on the display width
def split_text(text):
    # Calculate the maximum number of characters per line
    max_chars_per_line = tft.width // font.WIDTH
    
    # Split the text into lines based on the maximum characters per line
    lines = [text[i:i + max_chars_per_line] for i in range(0, len(text), max_chars_per_line)]
    print(f'Split text into lines: {lines}')
    return lines

# Main function to handle the conversation and display
def main():
    # Connect to Wi-Fi
    if connect_to_wifi(ssid, password):
        # Initialize the keyboard
        kb = KeyBoard()
        old_keys = set()
        user_input = ""  # Store the current input string
        initial_request_sent = False  # Flag to track if the initial request has been sent
        
        current_line = 0  # Track the current line in the response
        response_lines = []  # List of lines in the response text
        input_lines = []  # List of lines in the user input text
        
        # Keep track of user input on the screen
        display_text(0, MSG_INPUT_PROMPT, COLOR_INPUT)

        while True:
            # Get pressed keys from the keyboard
            keys = set(kb.get_pressed_keys())

            # If the set of keys changes, process the new keys
            if keys != old_keys:
                print(f'Keys changed. New keys: {keys}')
                
                # Find new keys pressed
                new_keys = keys - old_keys
                
                for key in new_keys:
                    print(f'Key pressed: {key}')
                    
                    if key == 'ENT':  # Enter key pressed
                        print('Enter key pressed')
                        
                        # Clear the whole screen once the first input is detected
                        if not initial_request_sent:
                            tft.fill(COLOR_BACKGROUND)
                            initial_request_sent = True
                            
                        display_text(0, MSG_SENDING_INPUT, COLOR_INPUT)

                        # Create request payload
                        data = {
                            "contents": [
                                {
                                    "role": "user",
                                    "parts": [{"text": user_input}]
                                }
                            ]
                        }
                        
                        # Convert to JSON format
                        json_data = ujson.dumps(data)

                        # Make the HTTP POST request
                        retry_count = 0
                        while retry_count < 3:
                            try:
                                print('Sending HTTP POST request...')
                                response = urequests.post(url, headers=headers, data=json_data)
                                print(f'Received HTTP response: {response.status_code}')
                                
                                if response.status_code == 200:
                                    # Handle successful response
                                    response_data = response.json()
                                    tft.fill(COLOR_BACKGROUND)
                                    
                                    # Parse response
                                    candidates = response_data.get('candidates', [])
                                    if candidates:
                                        first_candidate = candidates[0]
                                        content_parts = first_candidate.get('content', {}).get('parts', [])
                                        if content_parts:
                                            model_response_text = content_parts[0].get('text', '')
                                            print(f'Received model response text: "{model_response_text}"')
                                            
                                            # Split response into lines
                                            response_lines = split_text(model_response_text)
                                            
                                            # Reset current line index
                                            current_line = 0
                                            
                                            # Display the first few lines of response on lines 2 and onward
                                            for i in range(LINE_START_RESPONSE, len(response_lines) + LINE_START_RESPONSE):
                                                if i > LINE_START_RESPONSE + 6:
                                                    break
                                                display_text(i, response_lines[i - LINE_START_RESPONSE], COLOR_RESPONSE)

                                elif response.status_code == 500:
                                    # Handle server error 500
                                    print('Server Error 500')
                                    tft.fill(COLOR_BACKGROUND)
                                    display_text(0, MSG_SERVER_ERROR, COLOR_ERROR)
                                    
                                    # Retry sending the request after a delay
                                    print('Retrying in 5 seconds...')
                                    time.sleep(5)  # Wait for 5 seconds
                                    retry_count += 1
                                    continue  # Retry the loop

                                else:
                                    print(f'HTTP Error: {response.status_code}')
                                response.close()
                                break  # Exit the retry loop if request was successful

                            except Exception as e:
                                print(f'Error: {e}')
                                retry_count += 1
                                print('Retrying in 5 seconds...')
                                time.sleep(5)  # Retry after a delay
                                continue  # Retry the loop

                        # Reset user input and clear the input display
                        user_input = ""
                        input_lines = []  # Reset input lines
                        display_text(0, MSG_INPUT_PROMPT, COLOR_INPUT)

                    elif key == 'BSPC':  # Backspace key pressed
                        print('Backspace key pressed')
                        if user_input:
                            # Calculate the position of the last character
                            last_char_index = len(user_input) - 1
                            line_index = last_char_index // (tft.width // font.WIDTH)
                            char_index_in_line = last_char_index % (tft.width // font.WIDTH)

                            # Calculate the x, y coordinates for the last character
                            x = char_index_in_line * font.WIDTH
                            y = (line_index + LINE_START_INPUT) * font.HEIGHT
                            
                            # Clear the last character from the display
                            clear_char(x, y)

                            # Remove the last character from user input
                            user_input = user_input[:-1]
                            
                            # Split the user input
                            input_lines = split_text(user_input)
                            
                            # Clear the input display
                            display_text(0, MSG_INPUT_PROMPT, COLOR_INPUT)
                            
                            # Display split input text on line LINE_START_INPUT
                            for i, line in enumerate(input_lines):
                                display_text(i + LINE_START_INPUT, line, COLOR_INPUT)

                    elif key == 'SPC':  # Space key pressed
                        print('Space key pressed')
                        user_input += ' '
                        
                        # Split the user input
                        input_lines = split_text(user_input)
                        
                        # Clear the input display
                        display_text(0, MSG_INPUT_PROMPT, COLOR_INPUT)
                        
                        # Display split input text on line LINE_START_INPUT
                        for i, line in enumerate(input_lines):
                            display_text(i + LINE_START_INPUT, line, COLOR_INPUT)

                    elif key == 'LEFT':  # UP arrow key pressed
                        print('LEFT key pressed')
                        # Scroll up in response
                        if current_line > 0:
                            current_line -= 1
                            for i in range(LINE_START_RESPONSE, len(response_lines) + LINE_START_RESPONSE):
                                if i > LINE_START_RESPONSE + 6:
                                    break
                                display_text(i, response_lines[current_line + i - LINE_START_RESPONSE], COLOR_RESPONSE)

                    elif key == 'RIGHT':  # DOWN arrow key pressed
                        print('RIGHT key pressed')
                        # Scroll down in response
                        if current_line < len(response_lines) - 7:
                            current_line += 1
                            for i in range(LINE_START_RESPONSE, len(response_lines) + LINE_START_RESPONSE):
                                if i > LINE_START_RESPONSE + 6:
                                    break
                                display_text(i, response_lines[current_line + i - LINE_START_RESPONSE], COLOR_RESPONSE)

                    else:
                        # Add the pressed key to user input
                        user_input += key
                        print(f'Added key to user input: "{key}"')

                        # Split the user input
                        input_lines = split_text(user_input)
                        
                        # Clear the input display
                        display_text(0, MSG_INPUT_PROMPT, COLOR_INPUT)
                        
                        # Display split input text on line LINE_START_INPUT
                        for i, line in enumerate(input_lines):
                            display_text(i + LINE_START_INPUT, line, COLOR_INPUT)

                # Update old keys
                old_keys = keys

            # Add a short delay to avoid excessive polling
            time.sleep(0.1)

    else:
        print("Unable to establish Wi-Fi connection.")

# Run the main function
main()
