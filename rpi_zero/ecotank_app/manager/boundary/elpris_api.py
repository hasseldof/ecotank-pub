import requests
from datetime import datetime
from ..boundary.logger import logger


class ElprisAPI:
    """
    A boundary class that interacts with the actual Elpris API to fetch electricity price data.
    Uses the requests library to make HTTP GET requests the specified URL in the class initializer + "year/month-day_region.json"
    Has a rate limit built in to prevent spamming the API - this can be adjusted as needed.
    """

    def __init__(self):
        self.url = "https://www.elprisenligenu.dk/api/v1/prices/"
        self.last_fetch_at = None
        self.rate_limit = 1  # Rate limit in seconds


    def fetch_elpris(self, year, month, day, region):
        """
        Fetches electricity price data from the Elpris API for a specific date and region.

        Args:
            year (int): 4-digit format
            month (int): 2-digit format
            day (int): 2-digit format
            region (str): Either "DK1" or "DK2" for Aarhus/Vest and Koebenhavn/Oest, respectively.

        Returns:
            list of dicts: Returns a list of dictionaries, each containing the following key-value pairs:
                - 'DKK_per_kWh' (float): Price of electricity per kilowatt-hour in Danish Krone.
                - 'EUR_per_kWh' (float): Price of electricity per kilowatt-hour in Euros.
                - 'EXR' (float): Exchange rate used for DKK to EUR conversion.
                - 'time_start' (str): ISO 8601 formatted string representing the start time of the pricing interval.
                - 'time_end' (str): ISO 8601 formatted string representing the end time of the pricing interval.
            Returns None if an error occurred.
        """
        log_ctx = "Fetch Elpris (API):"

        # Attempt at a rate limit to prevent spamming the API
        if not self._can_fetch():
            logger.log(log_ctx, "Tried to fetch elpris data, but last fetched less than 1 seconds ago")
            return None

        # Try to request the data from the API
        try:
            response = requests.get(f"{self.url}{year}/{month}-{day}_{region}.json")
            response.raise_for_status()  # Raise an exception if we get an error response

            self.last_fetch_at = datetime.now()  # Update the last fetch time
            return response.json()  # Return the JSON data
        except requests.RequestException as e:
            logger.log(log_ctx, "Error fetching data from API", "WARNING", e)
            return None


    def _can_fetch(self):
        """
        Checks if data can be fetched from the API based on the last fetch time.

        Returns:
            bool: True if data can be fetched, False otherwise.
        """
        if self.last_fetch_at is not None:
            time_since_last_fetch = datetime.now() - self.last_fetch_at
            return time_since_last_fetch.total_seconds() >= self.rate_limit
        return True
