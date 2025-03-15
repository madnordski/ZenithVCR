/** index.h PART TWO
    @author J. King
    @date 2 March 2025
    @copy All rights reserved.
    @brief html for the Zenith Voice Controlled Radio.

    This is the general settings page.  Planned are two additional
    html pages, one for setting the urls for the streaming channels
    and another for the voice command id to task id mapping.
    
    $Revision: 1.4 $
    $Date: 2025/03/08 16:15:18 $
    use % tail +18 > tmp.html to preview in a web browser
*/

// index.html ready for substitution
const char index2_html[] PROGMEM = R"rawliteral(
<script>
  // " start javascript
var thisPage = window.location.ref;

document.addEventListener("DOMContentLoaded", function() {
  document.querySelectorAll(".pwclass").forEach(input => input.maxLength=79);
  document.querySelectorAll(".staclass").forEach(input => input.maxLength=255);
});    

window.onload = function() {
    // settings filled in by ESP32-S3
    
    // WiFi
    document.getElementById("ssid1").value = "%s";  // 1
    document.getElementById("pw1").value = "%s";
    document.getElementById("ssid2").value = "%s";
    document.getElementById("pw2").value = "%s";    // 4

    // Streaming Internet Radio Stations (13 stations)
    document.getElementById("sta55").value = "%s";   // 5
    document.getElementById("sta60").value = "%s";
    document.getElementById("sta65").value = "%s";
    document.getElementById("sta70").value = "%s";
    document.getElementById("sta80").value = "%s";
    document.getElementById("sta90").value = "%s";   // 10
    document.getElementById("sta100").value = "%s";  // 11
    document.getElementById("sta110").value = "%s";
    document.getElementById("sta120").value = "%s";
    document.getElementById("sta130").value = "%s";
    document.getElementById("sta140").value = "%s";
    document.getElementById("sta150").value = "%s";  // 16
    
    document.getElementById("sta160").value = "%s";  // 17

    // 4 wifi values plus 13 stations = 17 items
}

function sendBtn(item, reload) {
    let server = new XMLHttpRequest();
    server.open("GET", "/?" + item.id + "=on", true);
    server.send();
    if ( reload ) location.reload();
}

function togglepw(ob, pw) {
  pwOb = document.getElementById(pw);
  if ( pwOb.type == "password" ) {
    pwOb.type = "text";
    ob.value = "1";
  }
  else {
    pwOb.type = "password";
    ob.value = "0";
  }
}
      
</script>
    
</body></html>
)rawliteral";
