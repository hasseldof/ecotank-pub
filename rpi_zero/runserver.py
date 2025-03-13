import subprocess
from ecotank_app.manager.boundary.logger import logger

def run_server():
    """
    Entrance point to the EcoTank app for the RPi Zero W.
    Runs the webserver (flask application) and SystemManager (data and logic management) processes.

    It uses the subprocess module to create separate processes for each script.
    The processes can interact through the shared SQLite database.
    
    The output of the processes is captured and printed to the console.

    Note: 
        Make sure to set the correct paths for the Python interpreter and the script files.
        Create a virtual environment and install the required packages before running this script (requirements.txt)
    """
    logger.log("Startup:", "Starting the EcoTank app..")
    
    python_path = "rpi_zero/env/bin/python"
    webserver_cmd = [python_path, "rpi_zero/ecotank_app/webserver.py"]
    system_manager_cmd = [python_path, "rpi_zero/ecotank_app/system_manager.py"]
    
    webserver_process = subprocess.Popen(webserver_cmd)
    manager_process = subprocess.Popen(system_manager_cmd)
    
    web_stdout, web_stderr = webserver_process.communicate()
    mgr_stdout, mgr_stderr = manager_process.communicate()
    print("WEB_STDOUT:", web_stdout)
    print("WEB_STDERR:", web_stderr)
    print("MGR_STDOUT:", mgr_stdout)
    print("MGR_STDERR:", mgr_stderr)

if __name__ == '__main__':
    run_server()
