from sqlalchemy import (
    Column,
    Integer,
    String,
    DateTime,
    Float,
    ForeignKey,
    Time,
    UniqueConstraint,
    Boolean,
    create_engine,
)
from sqlalchemy.orm import sessionmaker, scoped_session, relationship
from sqlalchemy.orm.exc import NoResultFound
from sqlalchemy.ext.declarative import declarative_base
from datetime import datetime
from manager.boundary.logger import logger
from os import path

Base = declarative_base()

DB_NAME = "database.db"
engine = create_engine("sqlite:///" + path.join(path.dirname(__file__), "instance", DB_NAME))
session_factory = sessionmaker(bind=engine)
Session = scoped_session(session_factory)


class UserSettings(Base):
    """
    Represents the user settings for the application. Only a single instance of this class should exist in the database.
    To ensure this, use the `get_user_settings` function to get the user settings object.

    Attributes:
        id (int): The unique identifier for the user settings.
        min_temp (float): The minimum temperature set by the user.
        std_temp (float): The standard temperature set by the user.
        high_temp (float): The high temperature set by the user.
        price_treshold (float): The electricity price threshold set by the user.
        price_region (str): The electricity price region set by the user.
        time_intervals (list): The list of time intervals associated with the user settings.
    """

    __tablename__ = "user_settings"
    id = Column(Integer, primary_key=True)
    min_temp = Column(Integer, nullable=False, default=20)
    std_temp = Column(Integer, nullable=False, default=30)
    high_temp = Column(Integer, nullable=False, default=40)
    price_threshold = Column(Float, nullable=False, default=1.0)
    price_region = Column(String(6), nullable=False, default="DK1")
    days_to_fetch = Column(Integer, nullable=False, default=2)  # Add to settings page
    time_intervals = relationship("TimeInterval", cascade="all, delete-orphan")


class TimeInterval(Base):
    """
    Represents a time interval in the database. Each time interval is associated with the user settings object.
    Multiple time intervals can be associated with a single user settings object.
    
    Attributes:
        id (int): The unique identifier for the time interval.
        start_time (datetime.time): The start time of the interval.
        end_time (datetime.time): The end time of the interval.
        user_settings_id (int): The foreign key referencing the associated user settings.
    """

    __tablename__ = "time_interval"
    id = Column(Integer, primary_key=True)
    start_time = Column(Time, nullable=False)
    end_time = Column(Time, nullable=False)
    user_settings_id = Column(Integer, ForeignKey("user_settings.id"))


class SystemData(Base):
    """
    Represents the system data table in the database. Only a single instance of this class should exist in the database.
    To ensure this, use the `get_system_data` function to get the system data object.

    Attributes:
        id (int): The primary key of the table.
        sys_power (bool): The status of the system power.
        setpoint (float): The set point temperature for the system.
        water_temp (float): The current temperature of the water.
        water_level (float): The current level of the water.

    """

    __tablename__ = "system_data"
    id = Column(Integer, primary_key=True)
    sys_power = Column(Boolean, nullable=False, default=False)
    setpoint = Column(Integer, nullable=False, default=30.0)
    water_temp = Column(Float, nullable=False, default=0.0)
    water_level = Column(Integer, nullable=False, default=0)


class OverrideSettings(Base):
    """
    Represents the "opvarm vand manuelt" state and settings for the ecotank application.
    The user can toggle the state on the UI and specify whether high temperature is allowed during manual period.

    Attributes:
        id (int): The unique identifier for the table.
        start_time (datetime): The start time for the override period.
        end_time (datetime): The end time for the override period.
        toggled_on (bool): Indicates whether manuel opvarmning is toggled on or off.
        allow_high_temp (bool): Indicates whether high temperature is allowed while manual state is toggled on.
    """

    __tablename__ = "override_settings"
    id = Column(Integer, primary_key=True)
    start_time = Column(DateTime(timezone=True)) 
    end_time = Column(DateTime(timezone=True)) # Create button to add 20 minutes to end time, show on dashboard
    toggled_on = Column(Boolean, nullable=False, default=False) # Show on dashboard with button to toggle
    allow_high_temp = Column(Boolean, nullable=False, default=False) # Add to settings page


class ElectricityPrice(Base):
    """
    Represents the electricity price at a specific date and hour.
    There will be multiple instances of this class in the database, each representing the price at a specific date and time.

    Attributes:
        id (int): The unique identifier for the electricity price entry.
        DKK_per_kWh (float): The price in Danish Kroner per kilowatt-hour.
        EUR_per_kWh (float): The price in Euros per kilowatt-hour.
        EXR (float): The exchange rate between Danish Kroner and Euros.
        region (str): The region for which this price is applicable.
        time_start (datetime): The start time of the price interval.
        time_end (datetime): The end time of the price interval.
    """

    __tablename__ = "electricity_price"
    id = Column(Integer, primary_key=True)
    DKK_per_kWh = Column(Float, nullable=False)
    EUR_per_kWh = Column(Float, nullable=False)
    EXR = Column(Float, nullable=False)
    region = Column(String(5), nullable=False)
    time_start = Column(DateTime)
    time_end = Column(DateTime)

    __table_args__ = (UniqueConstraint("region", "time_start", "time_end", name="unique_region_datetime"),)


def create_database():
    """
    Creates the database if it doesn't already exist.

    This function checks if the database file exists in the 'instance' directory.
    If the file doesn't exist, it creates the database by calling the `create_all` method
    of the `Base.metadata` object.

    Note: The `engine` and `DB_NAME` variables should be defined before calling this function.
    """
    basedir = path.abspath(path.dirname(__file__))
    if not path.exists(path.join(basedir, "instance", DB_NAME)):
        logger.log("Database:", "Creating database..")
        try:
            Base.metadata.create_all(engine)
            logger.log("Database:", "Database created successfully.")
        except Exception as e:
            logger.log("Database:", "Failed to create database.", "ERROR", e)


def get_system_data(session):
    """
    Get the entire system data object. If no system data object exists, a new one is created.
    Use this function for all queries to the system data table, even if only a single attribute is needed or changed.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.

    Returns:
        SystemData: The system data object.
    """
    try:
        return session.query(SystemData).one()
    except NoResultFound:
        new_system_data = SystemData()
        session.add(new_system_data)
        session.commit()
        return new_system_data

def get_user_settings(session):

    """
    Get the entire user settings object. If no user settings object exists, a new one is created.
    Use this function for all queries to the user settings table, even if only a single attribute is needed or changed.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.

    Returns:
        UserSettings: The user settings object.
    """
    try:
        return session.query(UserSettings).one()
    except NoResultFound:
        new_user_settings = UserSettings()
        session.add(new_user_settings)
        session.commit()
        return new_user_settings


def get_override_settings(session):
    """
    Retrieves the override settings from the database. If no override settings are found, a new object is created.
    Use this function for all queries to the override settings table, even if only a single attribute is needed or changed.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.

    Returns:
        OverrideSettings: The override settings object.
    """
    try:
        return session.query(OverrideSettings).one()
    except NoResultFound:
        new_override_settings = OverrideSettings()
        session.add(new_override_settings)
        session.commit()
        return new_override_settings


def add_time_interval(session, start_time, end_time):
    """
    Add a new time interval to the user settings object.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.
        start_time (time): The start time of the time interval.
        end_time (time): THe end time of the time interval.

    Returns:
        None
    """
    user_settings = get_user_settings(session)
    new_time_interval = TimeInterval(start_time=start_time, end_time=end_time)
    user_settings.time_intervals.append(new_time_interval)
    session.commit()


def delete_time_interval(session, time_interval_id):
    """
    Deletes a time interval from the database.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.
        time_interval_id (int): The ID of the time interval to delete.

    Returns:
        None
    """
    time_interval = session.query(TimeInterval).filter_by(id=time_interval_id).one()
    session.delete(time_interval)
    session.commit()


def clear_all_time_intervals(session):
    """
    Deletes all time intervals for the current user.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.

    Returns:
        None
    """
    user_settings = get_user_settings(session)
    for time_interval in user_settings.time_intervals:
        session.delete(time_interval)
    user_settings.time_intervals = []  # Clear the list of time intervals
    session.commit()


def get_time_intervals(session):
    """
    Retrieve all time intervals from the database.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.

    Returns:
        A list of all time intervals in the database.
    """
    return session.query(TimeInterval).all()


def get_electricity_price(session, year, month, day, hour, region):
    """
    Retrieves the electricity price object for a specific date and time.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.
        year (int): The year of the desired date.
        month (int): The month of the desired date.
        day (int): The day of the desired date.
        hour (int): The hour of the desired time.
        region (str): The region for which to fetch the price.

    Returns:
        ElectricityPrice or None: The electricity price object for the specified date and time, or None if not found.
    """
    date_time = datetime(year, month, day, hour)

    try:
        return (
            session.query(ElectricityPrice)
            .filter(ElectricityPrice.time_start == date_time, ElectricityPrice.region == region)
            .one_or_none()
        )
    except NoResultFound:
        return None


def get_current_price(session, region):
    """
    Retrieves the electricty price object corresponding to the current hour from the database.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.

    Returns:
        The electricity price object corresponding to the current hour, or None if no object is found.
    """
    current_hour: datetime = datetime.now().replace(minute=0, second=0, microsecond=0)
    try:
        return (
            session.query(ElectricityPrice)
            .filter(ElectricityPrice.time_start == current_hour, ElectricityPrice.region == region)
            .one_or_none()
        )
    except NoResultFound:
        return None


def get_electricity_prices(session, region):
    """
    Retrieves all electricity price objects for a specified region from the database.

    Parameters:
        session (Session): The SQLAlchemy session to use for the query.
        region (str): The region for which to retrieve electricity prices.

    Returns:
        A list of all electricity price objects in the database for the specified region.
    """
    return session.query(ElectricityPrice).filter(ElectricityPrice.region == region).all()
