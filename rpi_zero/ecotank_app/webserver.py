from os import environ
from webserver import create_app
from manager.boundary.logger import logger

log_ctx = "Webserver Process:"
logger.log(log_ctx, "Webserver process started..")
logger.log(log_ctx, "Setting HOST address and port..")

HOST = environ.get('SERVER_HOST', '0.0.0.0')
try:
    PORT = int(environ.get('SERVER_PORT', '5555')) 
except ValueError:
    PORT = 5555

logger.log(log_ctx, "Starting Flask application..")
create_app().run(HOST, PORT, debug=False)
