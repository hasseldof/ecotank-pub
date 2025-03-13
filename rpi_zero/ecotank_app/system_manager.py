import shared_db as db
import time
from manager.control.elpris_data_manager import ElprisDataManager
from manager.boundary.logger import logger
from manager.control.setpoint_manager import SetpointManager
from manager.boundary.arduino_interface import ArduinoIF


class SystemManager:
    """
    SystemManager is responsible for managing the higher-level system and rules logic.
    The run method contains the main loop.
    """

    def __init__(self):
        self.arduino_interface = ArduinoIF()
        self.elpris_manager = ElprisDataManager()
        self.setpoint_manager = SetpointManager()
        db.create_database()
        self.log_ctx = "SystemManager Process:"
        logger.log(self.log_ctx, "Initialization complete.")

    def _get_pwr_and_setpoint(self):
        """
        Get the current power status and setpoint from the database.
        Redundant function, but used to simplify main loop/not add redundant functions to shared_db.

        """
        log_ctx = "Get Power and Setpoint:"

        with db.Session() as session:
            try:
                system_data = db.get_system_data(session)
            except Exception as e:
                logger.log(log_ctx, "Error getting system data from database", "ERROR", e)
            return system_data.sys_power, system_data.setpoint

    def _set_temp_and_lvl(self, temp, lvl):
        """
        Set the water temperature and level in the database.
        Redundant function, but used to simplify main loop/not add redundant functions to shared_db.

        """
        log_ctx = "Set Temp and Lvl:"

        with db.Session() as session:
            try:
                system_data = db.get_system_data(session)
            except Exception as e:
                logger.log(log_ctx, "Error getting system data from database", "ERROR", e)
                return

            try:
                system_data.water_temp = temp
                system_data.water_level = lvl
                session.commit()
            except Exception as e:
                logger.log(log_ctx, "Error setting system data in database", "ERROR", e)
                return

    def _check_temperature_limit(self):
        """
        Check if the water temperature has reached the limit of 90 °C.
        If above the limit, flip the system power to 0 and log the event.

        """
        log_ctx = "Check Temperature:"

        with db.Session() as session:
            try:
                system_data = db.get_system_data(session)
            except Exception as e:
                logger.log(log_ctx, "Error querying database", "ERROR", e)
                return

            if system_data.water_temp > 90 and system_data.sys_power == 1:
                logger.log(
                    log_ctx,
                    f"Temperature limit reached! Current temperature: {system_data.water_temp} °C. Shutting down system..",
                    "CRITICAL",
                )
                system_data.sys_power = 0
                session.commit()

    def run(self):
        logger.log(self.log_ctx, "Starting main loop..")
        while True:
            # Check for missing electricity price data. See elpris_data_manager.py under "control"
            self.elpris_manager.fetch_missing_data()

            # Evaluate and set the setpoint temperature. See setpoint_manager.py under "control"
            self.setpoint_manager.update_setpoint()

            system_power, setpoint = self._get_pwr_and_setpoint()
            result = self.arduino_interface.exchange_data(system_power, setpoint)
            if result is None:
                logger.log(self.log_ctx, "Failed to exchange data with Arduino interface.","ERROR")
            else:
                water_temp, water_level = result
                self._set_temp_and_lvl(water_temp, water_level)

            self._check_temperature_limit()
            time.sleep(0.2)

if __name__ == "__main__":
    SystemManager().run()
