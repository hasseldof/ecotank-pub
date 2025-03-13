
function setSystemPower(state) {
    fetch('/set-system-power', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ state: state }),
    }).then((_res) => {
        window.location.href = '/';
    });
}


window.onload = function() {
    setInterval(function() {
      fetch('/get_system_data')
        .then(response => response.json())
        .then(data => {
          document.querySelector('#waterTemp').textContent = `Vandtemperatur er: ${Math.round(data.water_temp * 10) / 10}°C`;
          document.querySelector('#waterLevel').textContent = `Vandstand er: ${Math.round(data.water_level)}% (${Math.round((data.water_level*0.02)*100)/100}L)`;

          // Update the system power switch checkbox state
          var sysPowerCheckbox = document.getElementById('customSwitch1');
          sysPowerCheckbox.checked = data.sys_power;
        });
    }, 1000); // Opdaterer hvert sekund
  }