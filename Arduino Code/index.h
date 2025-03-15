/** index.h PART ONE
    @author J. King
    @date 2 March 2025
    @copy All rights reserved.
    @brief html for the Zenith Voice Controlled Radio.

    This is the general settings page.  Planned are two additional
    html pages, one for setting the urls for the streaming channels
    and another for the voice command id to task id mapping.
    
    $Revision: 1.12 $
    $Date: 2025/03/11 20:11:40 $
    use % tail +18 > tmp.html to preview in a web browser
*/

// index.html ready for substitution
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="shortcut icon" href="data:," />
  
<style>

table.settings td { font-size: 24px; text-align:left; }
table.readings td {text-align: center; }

.pwclass {
    width: 60ch;  /* Matches size="60" */
    max-width: 100%;
}

.staclass {
    width: 60ch;  /* Matches size="60" */
    max-width: 100%;
}

</style>
</head>
<body>
  <h1><a href="/">Zenith General Settings</a></h1>
  
  <p>The Zenith Voice Controlled Radio was created by J. King, 6 March
  2025. The radio was originally
    a <a target="blank"
	 href="https://www.radiomuseum.org/r/zenith_4g903_ch4f40.html">Zenith
  4G903</a> which was first produced in 1949.</p>

  <p align="left">At least one access point must be specified. Only
  2.4 gz WiFi networks are supported.  <i>Important:</i> Only one
  browser at a time can access this page.</p>

<form method="post">
<table class="settings" align="center">
  <tr><td colspan=2 align="center">WiFi Settings</td></tr>
  <tr><th>Access Point 1 (SSID)</th>
    <td><input type="text" id="ssid1" name="ssid1" maxlength=31></td></tr>
    <tr><th>Passphrase</th>
    <td><input type="password" id="pw1", name="pw1" class="pwclass">
      <input type="button" value="&#x1f441;"
	     onclick="togglepw(this, 'pw1');"></td></tr>
  <tr><th>Access Point 2 (SSID)</th>
    <td><input type="text" id="ssid2" name="ssid2" maxlength=31></td></tr>
    <tr><th>Passphrase</th>
    <td><input type="password" id="pw2", name="pw2" class="pwclass">
      <input type="button" value="&#x1f441;"
	     onclick="togglepw(this, 'pw2');"></td></tr>
  <tr><td></td><td><input type="submit" value="Update">
      <input type="button" value="Restart" id="Restart"
           onclick="sendBtn(this, false); alert('The Zenith is restarting. You can close this window.');">
  </td></tr>
  
  <tr><td colspan=2 align="left"><font size="-1">1. Update the WiFi
  settings for your network.<br> 2. Restart the Zenith<br> 3. Visit
  http://zenith.local to make changes to the stations.</font></td></tr>

</table>
</form>

<hr>

<h3>The settings below are best configured using a computer. After
  restarting the Zenith Voice Controlled Radio, visit
  <a href="http://zenith.local">http://zenith.local</a>.</h3>

<p>In addition, you can specify up to 13 Internet Radio Station URLs
  which correspond to the dial numbers (55kz to 160kz). Keep in mind
  that there are voice commands associated with some of these
  stations. You can change a station url that is associted with a
  voice command, it just makes that voice command obsolete.  A
  complete list of the trained voice commands is provided below. In
  the table below, <sup>*</sup> indicates stations with associated
  voice commands.</p>

  <p>You can find compatible streaming services from these sources:</p>
  <ul>
    <li><a href="https://fmstream.org/index.php?c=FT" target="blank">
      https://fmstream.org/index.php?c=FT</a>
    <li><a href="https://streamurl.link/" target="blank">
      https://streamurl.link/</a>
  </ul>
  
<form method="post">
  <table class="settings" align="center">
    <title>Streaming Internet Radio Stations</title>
    <tr><th>Dial kHz</th><th>Station Streamming URL</th></tr>
    <tr><td>55<sup>*</sup>
      </td><td><input type="text" id="sta55" name="sta55" class="staclass">
		      </td></tr>
    <tr><td>60<sup>*</sup>
      </td><td><input type="text" id="sta60" name="sta60" class="staclass">
		      </td></tr>
    <tr><td>65<sup>*</sup>
      </td><td><input type="text" id="sta65" name="sta65" class="staclass">
		      </td></tr>
    <tr><td>70<sup>*</sup>
      </td><td><input type="text" id="sta70" name="sta70" class="staclass">
		       </td></tr>
    <tr><td>80<sup>*</sup>
      </td><td><input type="text" id="sta80" name="sta80" class="staclass">
		       </td></tr>
    <tr><td>90<sup>*</sup>
      </td><td><input type="text" id="sta90" name="sta90" class="staclass">
		       </td></tr>
    <tr><td>100<sup>*</sup>
      </td><td><input type="text" id="sta100" name="sta100" class="staclass">
		       </td></tr>
    <tr><td>110<sup>*</sup>
      </td><td><input type="text" id="sta110" name="sta110" class="staclass">
		       </td></tr>
    <tr><td>120</td>
      <td><input type="text" id="sta120" name="sta120" class="staclass">
		       </td></tr>
    <tr><td>130</td>
      <td><input type="text" id="sta130" name="sta130" class="staclass">
		  </td></tr>
    <tr><td>140</td>
      <td><input type="text" id="sta140" name="sta140" class="staclass">
		  </td></tr>
    <tr><td colspan=2><b>The last two stations stream local content
	  only<sup>++</sup>.</b></td></tr>
    <tr><td>150<sup>*</sup>
      </td><td><input type="text" id="sta150" name="sta150" class="staclass">
		  </td></tr>
    <tr><td>160</td>
      <td><input type="text" id="sta160" name="sta160" class="staclass">
		  </td></tr>
    <tr><td colspan=2>
	<input type="submit" value="Station Update" id="StaUpdate">
    </td></tr>
  </table>
</form>

<p><sup>++</sup>The code needed to stream your local content is on
    github (https://github.com/madnordski/ZenithVCR) and on a usb
    flash drive taped inside the radio.</p>

<h2>Voice Commands</h2>

<p align="left">Although you can't change the voice command to task
  mapping, it is displayed here. Before the Zenith will recognize a
  command, you'll need to mute the radio and say "Hello, Robot".
  After the Zenith quietly replies, you can speak any of the following
  commands to have it take the associated action.</p>
  
<table align="center">
  <title>Voice Commands and Task Mapping</title>
  <tr><th>You Say</th><th>The Zenith Performs Task</th></tr>
  <tr><td>Volume up</td><td>
      Turns up the volume</td></tr>
  <tr><td>Volume down</td><td>
      Turns down the volume</td></tr>
  <tr><td>Change volume to maximum</td><td>
      Sets volume to maximum</td></tr>
  <tr><td>Change volume to minimum</td><td>
      Sets volume to minimum</td></tr>
  <tr><td>Change volume to medium</td><td>
      Sets volume to the middle</td></tr>
  <tr><td>Station up</td><td>
      Moves the dial up one station</td></tr>
  <tr><td>Station down</td><td>
      Moves the dial down one station</td></tr>
  <tr><td>Pause radio</td><td>
      Pauses streamming radio</td></tr>
  <tr><td>Resume radio</td><td>
      Resumes streamming radio</td></tr>
  <tr><td>Turn off radio</td><td>
      Turns off radio, disables voice commands</td></tr>
  <tr><td>Turn on radio</td><td>
      Turns on radio, enables voice commands</td></tr>
  <tr><td>Latin Radio</td><td>
      Tunes into the station at 60 kHz on the dial</td></tr>
  <tr><td>Play classical music</td><td>
      Tunes into 65 kHz</td></tr>
  <tr><td>Play jazz music</td><td>
      Tunes into 110 kHz</td></tr>
  <tr><td>Wisconsin Public Radio</td><td>
      Tunes into 90 kHz</td></tr>
  <tr><td>NPR</td><td>
      Tunes into 80 kHz</td></tr>
  <tr><td>CBC Music</td><td>
      Tunes into 110 kHz</td></tr>
  <tr><td>CBC Talk</td><td>
      Tunes into 100 kHz</td></tr>
  <tr><td>Prairie Public Radio</td><td>
      Tunes into 70 kHz</td></tr>
  <tr><td>Stream Local</td><td>
      Tunes into 120 kHz</td></tr>
  <tr><td>How's Marvin</td><td>Not yet implemented</td></tr>
</table>
)rawliteral";
