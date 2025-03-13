import serial
import struct
import select
import atexit
import re
from ..boundary.logger import logger


class ArduinoIF:
    """
    Boundary class for interfacing with the Arduino through UART.
    """
    
    serial_port = "/dev/ttyACM0"
    baud_rate = 250000  # Must match the baud rate of the atmega2560.
    start_id_byte = b'\x7E'
    stop_id_byte = b'\x7D'
    setpoint_id_byte = b'\x5C'
    system_power_id_byte = b'\x88'
    temperature_id_byte = b'\x3A'
    water_level_id_byte = b'\x2F'
    dummy_byte = b'\xFF'

    # Corresponding enum values in the Arduino code
    # enum Byte : uint8_t {
    # 	START_BYTE = 0x7E,
    # 	STOP_BYTE = 0x7D,
    # 	SETPOINT = 0x5C,
    # 	SYSTEM_POWER = 0x88,
    # 	TEMPERATURE = 0x3A,
    # 	DISTANCE = 0x2F,
    # 	DUMMY_BYTE = 0xFF
    # };

    def __init__(self):
        self.ser = None
        self._open_serial_port()


    def _open_serial_port(self):
        log_ctx = "Open Serial Port:"
        attempts = 3  # Number of attempts to open the serial port

        for attempt in range(attempts):
            try:
                self.ser = serial.Serial(self.serial_port, self.baud_rate)
                atexit.register(self._close_serial_port)
                logger.log(log_ctx, f"Successfully opened on attempt {attempt + 1}", "INFO")
                break
            except serial.SerialException as e:
                logger.log(log_ctx, "Failed to open serial port", "ERROR", e)
                if attempt < attempts - 1:
                    logger.log(log_ctx, f"Retrying... ({attempt + 2}/{attempts})", "WARNING")
                else:
                    logger.log(log_ctx, "All attempts to open serial port failed.", "ERROR")


    def _close_serial_port(self):
        """
        This simply closes the serial port if it's open.
        The intention was to use this for handling the UART connection in a more controlled way,
        like opening and closing the port when needed or for handling a more dynamic setup 
        which allowed on-the-fly physical connection changes.
        
        Currently, it's only called when the program exits, so it's not really used.
        """
        log_ctx = "Close Serial Port:"

        if self.ser and self.ser.is_open:
            try:
                self.ser.close()
                logger.log(log_ctx, "Serial port closed successfully", "INFO")
            except Exception as e:
                logger.log(log_ctx, "Failed to close serial port", "ERROR", e)
            self.ser = None


    def exchange_data(self, system_power, setpoint):
        """
        Actual function used by the manager to exchange data with the Arduino.
        Here we use all the helper functions to send, receive and unpack data.

        Args:
            system_power (int): The system power value handed to send_data_frame.
            setpoint (float): The setpoint value handed to send_data_frame.
        """
        log_ctx = "Exchange Data:"

        # Attempt to reopen the port if not open
        if not self.ser or not self.ser.is_open:
            self._open_serial_port()  

        # If it's open, do the whole exchange process and return the water temperature and level
        if self.ser and self.ser.is_open:
            self._send_data_frame(system_power, setpoint)  # Send the current system data to the Arduino
            byte_stream = self._get_buffered_input()  # Get the bytes from the system UART input buffer
            frame = self._find_data_frame(byte_stream)  # Find a valid frame in the received bytes
            if frame is None:
                logger.log(log_ctx, "No valid frame found", "ERROR")
                return
            water_temp, water_level = self._unpack_data_frame(frame)  # Unpack the frame and get the water temperature and level
            return water_temp, water_level
        else:
            logger.log(log_ctx, "Cannot exchange data, serial port not open", "ERROR")
            return


    def _unpack_data_frame(self, response):
        log_ctx = "Unpack Data Frame:"

        if len(response) != 9:
            logger.log(log_ctx, "Trying to handle invalid frame size", "ERROR")
            return

        water_level = None
        water_temp = None

        for i in range(1, 9):
            if response[i] == self.water_level_id_byte[0]:
                water_level = response[i + 1]
            elif response[i] == self.temperature_id_byte[0]:
                water_temp = struct.unpack("<f", response[i + 1:i + 5])[0]

        if water_level is not None and water_temp is not None:
            return water_temp, water_level
        else:
            logger.log(log_ctx, "Data missing in the frame", "ERROR")


    def _send_data_frame(self, system_power, setpoint):
        """
        Sends a data frame to the Arduino.

        Args:
            system_power (int): The system power value to be sent.
            setpoint (float): The setpoint value to be sent.

        Returns:
            None

        Raises:
            Exception: If there is an error sending bytes to the Arduino.
        """
        log_ctx = "Send Data Frame:"

        # Convert the system power to a bytes object
        system_power_byte = bytes([int(system_power)])
        # Convert the set point to 4 bytes, packed in little-endian format
        setpoint_bytes = struct.pack("<f", setpoint)
        bytes_sequence = self.start_id_byte + self.system_power_id_byte + system_power_byte + self.setpoint_id_byte + setpoint_bytes + self.stop_id_byte

        try:
            # Send the bytes to the Arduino
            self.ser.write(bytes_sequence)
            logger.log(log_ctx, f"Sent bytes: {bytes_sequence}", "DEBUG")  # Log the sent bytes
        except Exception as e:
            logger.log(log_ctx, "Error sending bytes to Arduino", "ERROR", e)


    def _get_buffered_input(self):
        """
        Reads and returns the buffered input from the system UART input buffer.

        Returns:
            bytes: The received bytes from the Arduino.

        Raises:
            Exception: If there is an error reading bytes from the Arduino.
            
        Note:
            Currently, we don't actually handle waiting for the complete frame, 
            and once in a while the timing is off which results in a partial frame being returned.
        """
        log_ctx = "Get Buffered Input:"
        # self.ser.flushInput()
        response = None

        # Wait for the Arduino to respond with 5 retries (arbirtary number of retries)
        for _ in range(5):
            # Wait for data to be available on the serial port
            # select.select() waits for the serial port to be ready for reading with a defined timeout (0.2 seconds)
            readable, writable, exceptional = select.select([self.ser], [], [], 0.2)

            if readable:
                # Read all available data in the input buffer
                try:
                    response = self.ser.read(self.ser.in_waiting)
                    logger.log(log_ctx, f"Received bytes: {response}", "DEBUG")  # Log the received bytes
                except Exception as e:
                    logger.log(log_ctx, "Error reading bytes from Arduino", "ERROR", e)
                break  # Exit the loop once all the data has been read
            else:
                logger.log(log_ctx, "Timeout waiting for response from Arduino", "WARNING")
                continue

        # If no response was received, log an error and return None.
        if response is None:
            logger.log(log_ctx, "No response from Arduino", "ERROR")
            return

        # Else, return the received stream of bytes
        return response

    def _find_data_frame(self, response):
        """
        Searches for a valid data frame in the passed bytes.

        Args:
            response (bytes): The received bytes in which to search for a data frame.

        Returns:
            bytes: The last matched data frame found in the response, or None if no valid data frames are found.
        """
        log_ctx = "Find Data Frame:"

        if response is None:
            return None
        
        # Define a pattern to search for a valid frame in the received bytes.
        # The frame starts with the start_id_byte and ends with the stop_id_byte, and contains 7 bytes in between.
        frame_pattern = re.compile(
            self.start_id_byte + b".{7}" + self.stop_id_byte,
            re.DOTALL,  # Include all characters in the search (any byte value)
        )

        # Search for the pattern in the received bytes
        last_match = None
        for match in frame_pattern.finditer(response):
            last_match = match  # Keep updating last_match with the latest match found

        if last_match:
            frame = last_match.group(0)
            return frame  # The last matched frame is returned
        else:
            logger.log(log_ctx, "No valid data frames found", "DEBUG")
