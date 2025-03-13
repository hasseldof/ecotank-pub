"""
The flask application package.
"""
from flask import Flask
from flask_sqlalchemy import SQLAlchemy
from shared_db import DB_NAME, create_database, get_system_data
import os

db = SQLAlchemy()

def create_app():
    basedir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    app = Flask(__name__)
    app.config['SECRET_KEY'] = 'mysecretkey'
    app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///' + os.path.join(basedir, 'instance', DB_NAME)
    app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
    db.init_app(app)

    create_database()

    from .views import views
    from .auth import auth
    app.register_blueprint(views, url_prefix='/')
    app.register_blueprint(auth, url_prefix='/')

    @app.context_processor
    def inject_temperature_warning():
        water_temp = get_system_data(db.session).water_temp
        if water_temp and water_temp > 90:
            return {"temp_warning": "Warning: Water temperature is above 90 degrees!"}
        return {}

    return app
