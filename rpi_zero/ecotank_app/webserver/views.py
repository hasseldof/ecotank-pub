"""
Routes and views for the flask application.
"""
import shared_db
from . import db
from flask_login import UserMixin
from sqlalchemy.sql import func
from datetime import datetime, time
from flask import Blueprint, flash, jsonify, render_template, request, redirect, url_for
import json
from bokeh.plotting import figure
from bokeh.embed import components
from bokeh.resources import CDN
from bokeh.models import (
    DatetimeTickFormatter,
    DataRange1d,
    PanTool,
    WheelZoomTool,
    FixedTicker,
    Range1d,
    NumeralTickFormatter,
    DatetimeTicker,
)
from manager.boundary.logger import Logger
from math import floor, ceil

views = Blueprint('views', __name__, template_folder='templates')

@views.route('/')
@views.route('/home')
def home():
    """Renders the home page."""
    return render_template(
        'index.html',
        title='Home Page',
        year=datetime.now().year,
        system_data=shared_db.get_system_data(db.session),
        water_volume = shared_db.get_system_data(db.session).water_level*0.02,
        user_settings=shared_db.get_user_settings(db.session),
        override_settings=shared_db.get_override_settings(db.session),
    )


@views.route("/dashboard")
def dashboard():
    region = shared_db.get_user_settings(db.session).price_region
    price_objects = shared_db.get_electricity_prices(db.session, region)
    datetimes = [price_object.time_start for price_object in price_objects]
    prices = [price_object.DKK_per_kWh for price_object in price_objects]

    plot_width = 800  # width in pixels
    plot_height = 500  # height in pixels

    # Determining the y-axis range with padding
    min_price = min(prices)
    max_price = max(prices)

    # Adjust the start and end for ticks
    tick_start = floor(min_price * 10) / 10
    tick_end = ceil(max_price * 10) / 10

    # Generating ticks every 0.1 within the range
    y_ticks = [tick_start + i * 0.1 for i in range(int((tick_end - tick_start) / 0.1) + 1)]

    y_range = Range1d(start=tick_start, end=tick_end)

    # Creating the figure
    p = figure(
        title="Elpriser i DKK pr. kWh",
        x_axis_label="Tidspunkt",
        y_axis_label="DKK pr. kWh",
        x_axis_type="datetime",
        sizing_mode="scale_width",
        width=plot_width,
        height=plot_height,
        y_range=y_range,
        tools="pan",  # Include basic tools, excluding WheelZoomTool to customize it next
    )

    # Adding line renderer
    p.line(datetimes, prices, legend_label="DKK per kWh", line_width=2)

    p.xaxis.ticker = DatetimeTicker()
    # Formatting the datetime ticks on the x-axis
    p.xaxis.formatter = DatetimeTickFormatter(
        minutes=["%H:%M"],
        hours=["%d/%m %H:%M"],
        days=["%d/%m"],
        months=["%B %Y"],
        years=["%Y"],
    )

    p.yaxis[0].ticker = FixedTicker(ticks=y_ticks)
    p.yaxis[0].formatter = NumeralTickFormatter(format="0.0")

    # Add customized WheelZoomTool
    wheel_zoom = WheelZoomTool(dimensions="width")  # Enable zooming only on x-axis
    p.add_tools(wheel_zoom)
    p.toolbar.active_scroll = wheel_zoom  # Set the custom WheelZoomTool as the active scroll tool

    # Components for embedding the plot in the webpage
    script, div = components(p)

    return render_template(
        "dashboard.html",
        title="Dashboard",
        year=datetime.now().year,
        script=script,
        div=div,
        cdn_js=CDN.render_js(),
    )


@views.route('/log')
def log():
    try:
        with open('log.txt', 'r') as f:
            log_content = f.read()
    except FileNotFoundError:
        log_content = 'Log file not found.'
    except Exception as e:
        log_content = f'An error occurred: {e}'
    return render_template('log.html', log_content=log_content, year=datetime.now().year,)

@views.route('/clear_log')
def clear_log():
    with open('log.txt', 'w') as log:
        log.write('')
    return redirect(url_for('views.log'))


@views.route('/help')
def help():
    """Renders the about page."""
    return render_template(
        'help.html',
        title='About',
        year=datetime.now().year,
        message='Your application description page.'
    )

@views.route('/add_time_interval', methods=['POST'])
def add_time_interval():
    start = request.form.get('start')
    end = request.form.get('end')
    try:
        start_time = time.fromisoformat(start)
        end_time = time.fromisoformat(end)
        # Not done yet
        return redirect(url_for('index'))
    except ValueError:
        return "Invalid time format", 400
