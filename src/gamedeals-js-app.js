// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
  }
);

Pebble.addEventListener('appmessage', 
  function(e) {
    console.log("Requesting posts!");
        
    Pebble.sendAppMessage({
      "NUM_MENU_ITEMS": 2,
      "POSTS": "HELLO|WORLD",
    });
  }
);