


// Get all close buttons
var closeButtons = document.querySelectorAll('.list-group-item .badge');

// Add event listener to each close button
for (var i = 0; i < closeButtons.length; i++) {
  closeButtons[i].addEventListener('click', function() {
    // Get the parent list item
    var listItem = this.parentNode;

    // Get the corresponding hidden input
    var hiddenInput = listItem.querySelector('input[type="hidden"]');

    // Remove the list item and the hidden input
    intervalList.removeChild(listItem);
    document.querySelector('form').removeChild(hiddenInput);
  });
}







document.getElementById('addInterval').addEventListener('click', function() {
    var startTime = document.getElementById('startTime');
    var endTime = document.getElementById('endTime');
  
    if (startTime.value === '' || endTime.value === '') {
        return;
    }
    var intervalList = document.getElementById('intervalList');
    var listItem = document.createElement('li');
    listItem.classList.add('list-group-item');
    listItem.textContent = startTime.value + ' - ' + endTime.value;
  
    // Create a close icon for the list item
    var closeIcon = document.createElement('span');
    closeIcon.classList.add('badge', 'badge-secondary', 'float-right');
    closeIcon.innerHTML = '&times;';
    closeIcon.style.cursor = 'pointer';
    closeIcon.addEventListener('click', function() {
      intervalList.removeChild(listItem);
      document.querySelector('form').removeChild(hiddenInput);
    });
    listItem.appendChild(closeIcon);
  
    intervalList.appendChild(listItem);
  
    // Create a hidden input element for this time interval
    var hiddenInput = document.createElement('input');
    hiddenInput.type = 'hidden';
    hiddenInput.name = 'timeIntervals[]';
    hiddenInput.value = startTime.value + ' - ' + endTime.value;
  
    // Add the hidden input to the form
    document.querySelector('form').appendChild(hiddenInput);
  
    // Clear the input fields
    startTime.value = '';
    endTime.value = '';
  });