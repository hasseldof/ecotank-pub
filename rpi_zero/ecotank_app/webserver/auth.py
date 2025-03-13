from datetime import datetime, timedelta 
import json
from flask import Blueprint, render_template, request, flash, redirect, url_for, jsonify
import shared_db
from . import db
from manager.boundary.logger import logger

auth = Blueprint('auth', __name__)

@auth.route('/settings', methods=['GET', 'POST'])
def settings():
    if request.method == 'POST':
        new_min_temp = int(request.form.get('minTemp'))
        new_high_temp = int(request.form.get('highTemp'))
        new_std_temp = int(request.form.get('stdTemp'))

        price_threshold_input = request.form.get("electricityPriceLimit").replace(",", ".")
        try:
            new_price_threshold = float(price_threshold_input)
        except ValueError:
            flash(
                "Elprisgrænsen skal være et tal. Brug punktum eller komma som separator!", category="error"
            )
            return redirect(url_for("auth.settings"))
        new_price_region = request.form.get('electricityPriceRegion')
        time_intervals = request.form.getlist('timeIntervals[]')

        # Validations for temperatures
        if new_min_temp < 1 or new_min_temp > 95:
            flash('Minimumstemperatur skal være mellem 1 og 95 grader', category='error')
        elif new_high_temp < 1 or new_high_temp > 95:
            flash('Høj temperatur skal være mellem 1 og 95 grader', category='error')
        elif new_std_temp < 1 or new_std_temp > 95:
            flash('Standardstemperatur skal være mellem 1 og 95 grader', category='error')
        elif new_min_temp > new_std_temp:
            flash('Minimumstemperatur kan ikke være højere end standardstemperaturen', category='error')
        elif new_std_temp > new_high_temp:
            flash('Standardstemperatur kan ikke være højere end høj temperatur', category='error')
        else:
            user_settings = shared_db.get_user_settings(db.session)
            user_settings.min_temp = new_min_temp
            user_settings.high_temp = new_high_temp
            user_settings.std_temp = new_std_temp
            user_settings.price_threshold = new_price_threshold
            user_settings.price_region = new_price_region

            # Clear all time intervals
            shared_db.clear_all_time_intervals(db.session)
            # Add new time intervals
            for time_interval in time_intervals:
                # Split the time interval string into start and end time strings
                start_time_str, end_time_str = time_interval.split('-')
                # Convert the time strings to time objects
                start_time = datetime.strptime(start_time_str.strip(), '%H:%M').time()
                end_time = datetime.strptime(end_time_str.strip(), '%H:%M').time()

                shared_db.add_time_interval(db.session, start_time, end_time)

            db.session.commit()
            flash('Indstillingerne er gemt', category='success')

    user_settings = shared_db.get_user_settings(db.session)
    print(user_settings)
    return render_template("settings.html", existing_settings=shared_db.get_user_settings(db.session), year=datetime.now().year)


@auth.route('/set-system-power',methods=['POST'])
def setSystemPower():
    log_ctx = "System Power:"
    state = json.loads(request.data)
    stateToSet = state['state']
    system_data = shared_db.get_system_data(db.session)
    system_data.sys_power = stateToSet
    db.session.commit()
    
    if stateToSet == True:
        flash(f'System er nu tændt!', category='success')
        logger.log(log_ctx, "System is now turned on")
    else:
        flash(f'System er nu slukket!', category='error')
        logger.log(log_ctx, "System is now turned off")
    return jsonify({})

@auth.route('/get_system_data')
def getSystemData():
    # Her skal du hente de opdaterede systemdata
    system_data = {
        'water_temp': shared_db.get_system_data(db.session).water_temp,
        'water_level': shared_db.get_system_data(db.session).water_level,
        'sys_power': shared_db.get_system_data(db.session).sys_power,
    }
    return jsonify(system_data)


@auth.route('/manual-heating')
def manual_heating():
    manual_heating_state = shared_db.get_override_settings(db.session).toggled_on
    if manual_heating_state:
        shared_db.get_override_settings(db.session).toggled_on = False
        flash('Manuel opvarmning er slukket', category='error')
    else:
        shared_db.get_override_settings(db.session).toggled_on = True
        shared_db.get_override_settings(db.session).start_time = datetime.now()
        shared_db.get_override_settings(db.session).end_time = datetime.now() + timedelta(minutes=20)

        flash('Manuel opvarmning er tændt', category='success')
    db.session.commit()
    return redirect(url_for('views.home'))
