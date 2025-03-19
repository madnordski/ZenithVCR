/** index.h PART ONE
    @author J. King
    @date 2 March 2025
    @copy All rights reserved.
    @brief html for the Zenith Voice Controlled Radio.

    This is the general settings page.  Planned are two additional
    html pages, one for setting the urls for the streaming channels
    and another for the voice command id to task id mapping.
    
    $Revision: 1.14 $
    $Date: 2025/03/19 14:48:48 $
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

.tskclass {
    width: 4ch;
    max-width: 100%;
    type: text;
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

  <p align="left"><a target="blank"
    href="https://wiki.dfrobot.com/SKU_SEN0539-EN_Gravity_Voice_Recognition_Module_I2C_UART">Follow
    the instructions</a> for training the DFRobot SEN0539 and record
    the voice command id for each trained command.  Use the page below
    to associate each command with an action that the Zenith can take.
    Notice the first 5 commands in the table are preprogrammed in the
    DFRobot so you do not train those commands.</p>

<form method="post">
<table align="center">
  <title>Voice Commands and Task Mapping</title>
  <tr><th>Command Id</th><th>TID</th>
    <th>The Zenith Performs Task</th><th>Suggested Voice Command</th></tr>
  <tr><td>97</td><td>5</td>
    <td>Turns up the volume</td><td>Volume up</td></tr>
  <tr><td>98</td><td>6</td>
    <td>Turns down the volume</td><td>Volume down</td></tr>
  <tr><td>99</td><td>7</td>
    <td>Volume to maximum</td><td>Change volume to maximum</td></tr>
  <tr><td>100</td><td>8</td>
    <td>Volume to minimum</td><td>Change volume to minimum</td></tr>
  <tr><td>101</td><td>9</td>
    <td>Volume to medium</td><td>Change volume to medium</td></tr>
  <tr><td><input class="tskclass" id="tsk10" name="tsk10" value=5></td>
    <td>10</td>
    <td>Moves the dial up one station</td><td>Station up</td></tr>
  <tr><td><input class="tskclass" id="tsk11" name="tsk11" value=6></td>
    <td>11</td>
    <td>Moves the dial down one station</td><td>Station down</td></tr>
  <tr><td><input class="tskclass" id="tsk22" name="tsk22" value=17></td>
    <td>22</td>
    <td>Pause playback</td><td>Pause radio</td></tr>
  <tr><td><input class="tskclass" id="tsk23" name="tsk23" value=18></td>
    <td>23</td>
    <td>Resume playback</td><td>Resume radio</td></tr>
  <tr><td><input class="tskclass" id="tsk24" name="tsk24" value=19></td>
    <td>24</td>
    <td>Turns off the radio</td><td>Turn off radio</td></tr>
  <tr><td><input class="tskclass" id="tsk25" name="tsk25" value=20></td>
    <td>25</td>
    <td>Turns on the radio</td><td>Turn on radio</td></tr>
  <tr><td><input class="tskclass" id="tsk17" name="tsk17" value=12></td>
    <td>17</td>
    <td>Tunes to 60 kHz</td><td>Latin radio</td></tr>
  <tr><td><input class="tskclass" id="tsk19" name="tsk19" value=14></td>
    <td>19</td>
    <td>Tunes to 65 kHz</td><td>Play classical music</td></tr>
  <tr><td><input class="tskclass" id="tsk20" name="tsk20" value=15></td>
    <td>20</td>
    <td>Tunes to 140 kHz</td><td>Play jazz music</td></tr>
  <tr><td><input class="tskclass" id="tsk18" name="tsk18" value=13></td>
    <td>18</td>
    <td>Tunes to 55 kHz</td><td>Northern State Radio</td></tr>
  <tr><td><input class="tskclass" id="tsk16" name="tsk16" value=11></td>
    <td>16</td>
    <td>Tunes to 70 kHz</td><td>Prairie Public Radio</td></tr>
  <tr><td><input class="tskclass" id="tsk13" name="tsk13" value=8></td>
    <td>13</td>
    <td>Tunes to 80 kHz</td><td>NPR</td></tr>
  <tr><td><input class="tskclass" id="tsk12" name="tsk12" value=7></td>
    <td>12</td>
    <td>Tunes to 90 kHz</td><td>Wisconsin Public Radio</td></tr>
  <tr><td><input class="tskclass" id="tsk15" name="tsk15" value=10></td>
    <td>15</td>
    <td>Tunes to 100 kHz</td><td>CBC Talk</td></tr>
  <tr><td><input class="tskclass" id="tsk14" name="tsk14" value=9></td>
    <td>14</td>
    <td>Tunes to 110 kHz</td><td>CBC Music</td></tr>
  <tr><td><input class="tskclass" id="tsk21" name="tsk21" value=16></td>
    <td>21</td>
    <td>Tunes to 150 kHz</td><td>Stream Local</td></tr>
  <tr><td><input class="tskclass" id="tsk26" name="tsk26" value=21></td>
    <td>26</td>
    <td>Tunes to 160 kHz</td><td>Stream Podcast</td></tr>
    <tr><td colspan=4>
	<input type="submit" value="Command Update" id="TskUpdate"
	       onclick="alert('Close the browser to use the Zenith.');">
    </td></tr>
</table></form>

)rawliteral";
