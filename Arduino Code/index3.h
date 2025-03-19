/** index.h PART THREE
    @author J. King
    @date 2 March 2025
    @copy All rights reserved.
    @brief html for the Zenith Voice Controlled Radio.

    This is the general settings page.  Planned are two additional
    html pages, one for setting the urls for the streaming channels
    and another for the voice command id to task id mapping.
    
    $Revision: 1.2 $
    $Date: 2025/03/19 14:42:08 $
    use % tail +18 > tmp.html to preview in a web browser
*/

// index.html ready for substitution
const char index3_html[] PROGMEM = R"rawliteral(
// Begin command/task settings "
  document.getElementById("tsk10").value = "%d";
  document.getElementById("tsk11").value = "%d";
  document.getElementById("tsk12").value = "%d";
  document.getElementById("tsk13").value = "%d";
  document.getElementById("tsk14").value = "%d";
  document.getElementById("tsk15").value = "%d";
  document.getElementById("tsk16").value = "%d";
  document.getElementById("tsk17").value = "%d";
  document.getElementById("tsk18").value = "%d";
  document.getElementById("tsk19").value = "%d";
  document.getElementById("tsk20").value = "%d";
  document.getElementById("tsk21").value = "%d";
  document.getElementById("tsk22").value = "%d";
  document.getElementById("tsk23").value = "%d";
  document.getElementById("tsk24").value = "%d";
  document.getElementById("tsk25").value = "%d";
  document.getElementById("tsk26").value = "%d";
} // terminates onload() started in index2.h
						
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
