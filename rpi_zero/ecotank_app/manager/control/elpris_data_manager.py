from datetime import datetime, date, timedelta
import time
import shared_db as db
from ..boundary.elpris_api import ElprisAPI
from ..boundary.logger import logger


class ElprisDataManager(object):
    """
    Class for managing electricity price data. 
    
    Checks for missing data in the database based on the amount of days specified in the user settings.
    The days specified represents the days back in time to fetch data for.
    Only fetches data from the API if there is missing data in the database. Uses the ElprisAPI class.
    """

    def __init__(self):
        self.api = ElprisAPI()
        self.next_check_time = datetime.now()

    def fetch_missing_data(self) -> None:
        """
        Fetches missing electricity price data from the API and stores it in the database.
        """
        log_ctx = "Fetch Missing Data:"
        if datetime.now() < self.next_check_time:
            return

        with db.Session() as session:
            try:
                user_settings = db.get_user_settings(session)
                region = user_settings.price_region
            except Exception as e:
                logger.log(log_ctx, "Error querying database", "ERROR", e)
                return

            start_date: date = date.today() - timedelta(days=user_settings.days_to_fetch)
            end_date: date = date.today()
            if datetime.now().hour > 15:
                end_date += timedelta(days=1)

        missing_dates = sorted(self._check_for_missing_data(start_date, end_date, region))

        if not missing_dates:
            self.next_check_time = datetime.now() + timedelta(hours=1)
            return

        for missing_date in missing_dates:
            year = missing_date.year
            month = f"{missing_date.month:02d}"
            day = f"{missing_date.day:02d}"

            json_data = self.api.fetch_elpris(year, month, day, region)

            if json_data is not None:
                self._convert_json_and_commit(json_data, region)
            else:
                logger.log(log_ctx, f"Failed to fetch data for date {missing_date}", level="WARNING")
                break
            time.sleep(2)

    def _check_for_missing_data(self, start_date, end_date, region):
        """
        Checks for missing electricity price data in the specified date range.

        Args:
            start_date (date): The start date of the range.
            end_date (date): The end date of the range.

        Returns:
            list: A list of missing dates.
        """
        log_ctx = "Check For Missing Data:"
        logger.log(log_ctx, f"Checking for missing data from {start_date} to {end_date}")

        hour_to_check = 0
        missing_dates: list = []

        with db.Session() as session:
            current_date: date = start_date
            while current_date <= end_date:
                try:
                    if not db.get_electricity_price(
                        session=session,
                        year=current_date.year,
                        month=current_date.month,
                        day=current_date.day,
                        hour=hour_to_check,
                        region=region,
                    ):
                        missing_dates.append(current_date)
                        logger.log(log_ctx, f"Missing data for dates: {current_date}")
                except Exception as e:
                    logger.log(log_ctx, "Error querying database", "ERROR", e)
                    return None
                current_date += timedelta(days=1)
        return missing_dates

    def _convert_json_and_commit(self, json_data, region) -> None:
        """
        Converts JSON data to ElectricityPrice objects and commits them to the database.

        Args:
            json_data: The JSON data to convert and commit.
        """
        log_ctx = "Convert JSON and Commit:"
        entries: list = []
        for entry in json_data:
            entries.append(
                db.ElectricityPrice(
                    DKK_per_kWh=entry["DKK_per_kWh"],
                    EUR_per_kWh=entry["EUR_per_kWh"],
                    EXR=entry["EXR"],
                    region=region,
                    time_start=datetime.fromisoformat(entry["time_start"]).replace(tzinfo=None),
                    time_end=datetime.fromisoformat(entry["time_end"]).replace(tzinfo=None),
                )
            )
        logger.log(log_ctx, f"Time_start format after conversion: {entries[0].time_start}", level="DEBUG")

        with db.Session() as session:
            try:
                session.add_all(entries)
                session.commit()
                logger.log(log_ctx, "Successfully committed electricity price entries to database")
            except Exception as e:
                logger.log(log_ctx, "Error committing to database", "ERROR", e)
