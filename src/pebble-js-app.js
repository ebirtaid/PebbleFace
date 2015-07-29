var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var api_key = "";
  var url = "https://api.forecast.io/forecast/" + api_key + "/" +
      pos.coords.latitude + "," + pos.coords.longitude;

  // Send request to forecast.io
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      //Documentation at https://developer.forecast.io/docs/v2
      // Temperature
      var temperature = json.currently.apparentTemperature;
      console.log("Temperature is " + temperature);

      // Conditions
      var conditions = json.currently.summary;      
      console.log("Conditions are " + conditions);
      
      // hourly
      var hourly = json.minutely.summary;      
      console.log("hourly forecast is " + hourly);
      
      //daily
      var daily = json.hourly.summary;      
      console.log("daily forecast is " + daily);
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions,
        "KEY_HOURLY": hourly,
        "KEY_DAILY": daily
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);
