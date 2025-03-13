from datetime import datetime


class Logger(object):
    """Basic logging class that writes log messages to a file and prints the log message to console."""

    # Mapping log levels to numeric values
    LOG_LEVELS = {"DEBUG": 10, "INFO": 20, "WARNING": 30, "ERROR": 40, "CRITICAL": 50}

    def __init__(self, debug_enabled=False):
        self.log_file = "log.txt"
        self.debug_enabled = debug_enabled
        self.current_level = self.LOG_LEVELS["DEBUG"] if debug_enabled else self.LOG_LEVELS["INFO"]

    def log(self, context, message, level="INFO", error=None):
        """
        Logs messages with optional error reporting and context information.

        Parameters:
            context (str): Describes the part of the application or the specific operation where the log is being generated.
            message (str): The primary log message.
            level (str): The severity level of the log (default='INFO', 'WARNING', 'ERROR', 'CRITICAL', 'DEBUG').
            error (Exception, optional): Exception object to log, if any.
        """

        # Check if the log level is above or equal to the current threshold
        if self.LOG_LEVELS[level] >= self.current_level:
            full_message = f"{context} - {message}"
            if error:
                full_message += f" | Exception: {str(error)}"  # Append error message if provided

            log_message = f"{datetime.now().isoformat(timespec='seconds')} - {level} - {full_message}"

            # Write to file and console
            try:
                with open(self.log_file, "a") as file:
                    file.write(log_message + "\n")
            except Exception as e:
                print(f"Failed to write to log file: {e}")

            print(log_message)

# Create a global instance
logger = Logger(debug_enabled=False)
