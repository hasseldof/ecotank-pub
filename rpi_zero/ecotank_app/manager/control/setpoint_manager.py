from datetime import datetime
from typing import List
from ..boundary.logger import logger
import shared_db as db


class SetpointManager(object):
    """
    The SetpointManager class is responsible for managing the setpoint temperature based on various conditions.

    Methods:
        - check_time_intervals: Checks if the current time is within any defined time intervals in the database.
        - check_manual_override: Checks if manual override is enabled and within the specified time interval.
        - check_elpris_threshold: Checks if the current electricity price is below the user-defined threshold.
        - update_setpoint: Sets the setpoint temperature based on the conditions checked by the above methods.
    """

    def _check_time_intervals(self) -> bool:
        log_ctx = "Check Time Intervals:"
        with db.Session() as session:
            try:
                time_intervals: List[db.TimeInterval] = db.get_time_intervals(session)
            except Exception as e:
                logger.log(log_ctx, "Error querying database", "ERROR", e)
                return False

        if time_intervals is None:
            return False

        for interval in time_intervals:
            if interval.start_time <= datetime.now().time() <= interval.end_time:
                return True
        return False

    def _check_manual_override(self) -> bool:
        log_ctx = "Check Manual Override:"
        with db.Session() as session:
            try:
                override: db.OverrideSettings = db.get_override_settings(session)
            except Exception as e:
                logger.log(log_ctx, "Error querying database", "ERROR", e)
                return False

            if override.toggled_on:
                if override.start_time <= datetime.now() <= override.end_time:
                    return True
                else:
                    override.toggled_on = False
                    logger.log(log_ctx, "Manuel opvarmning turned off: outside of time interval")
                    try:
                        session.commit()
                    except Exception as e:
                        logger.log(log_ctx, "Error committing to database", "ERROR", e)
            return False

    def _check_elpris_threshold(self) -> bool:
        log_ctx = "Check Elpris Threshold:"
        with db.Session() as session:
            try:
                user_settings: db.UserSettings = db.get_user_settings(session)
                current_price: db.ElectricityPrice | None = db.get_current_price(session, user_settings.price_region)
            except Exception as e:
                logger.log(log_ctx, "Error getting data from database", "ERROR", e)
                return False

        if current_price is None:
            logger.log(log_ctx, "No price entry in database for the current hour", level="WARNING")
            return False

        if current_price.DKK_per_kWh < user_settings.price_threshold:
            return True
        else:
            return False

    def update_setpoint(self):
        log_ctx = "Set Setpoint:"

        in_time_interval: bool = self._check_time_intervals()
        manual_override: bool = self._check_manual_override()
        price_under_threshold: bool = self._check_elpris_threshold()

        with db.Session() as session:
            try:
                system_data: db.SystemData = db.get_system_data(session)
                user_settings: db.UserSettings = db.get_user_settings(session)
                override_settings: db.OverrideSettings = db.get_override_settings(session)
                override_allow_high_temp: db.Column[bool] = override_settings.allow_high_temp
            except Exception as e:
                logger.log(log_ctx, "Error querying database", "ERROR", e)
                return

            if price_under_threshold:
                if manual_override and not override_allow_high_temp:
                    setpoint = user_settings.std_temp
                    temperature_name = "standard temperature"
                    log_msg = (
                        "Electricity price below configured threshold, manual heating enabled but high temp not allowed"
                    )
                else:
                    setpoint = user_settings.high_temp
                    temperature_name = "high temperature"
                    log_msg = "Electricity price below configured threshold"
            elif manual_override:
                setpoint = user_settings.std_temp
                temperature_name = "standard temperature"
                log_msg = "Manual heating enabled"
            elif in_time_interval:
                setpoint = user_settings.std_temp
                temperature_name = "standard temperature"
                log_msg = "Within configured time interval"
            else:
                setpoint = user_settings.min_temp
                temperature_name = "minimum temperature"
                log_msg = "No rules matched for the current moment"

            if setpoint == system_data.setpoint:
                return

            logger.log(log_ctx, f"{log_msg} - setpoint set to {temperature_name}: {setpoint} °C")
            system_data.setpoint = setpoint

            try:
                session.commit()
            except Exception as e:
                logger.log(log_ctx, "Error committing to database", "ERROR", e)
