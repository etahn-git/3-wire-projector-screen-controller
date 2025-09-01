# 3-Wire Projector Screen Controller
A project made with an ESP32 and a dual channel relay to replace a 3 way projector screen switch and instead automatically put the screen down when the USB port from the projector provides 5v power. Manual control is also possible VIA http webpage or http API.
<br>
## Wiring Diagram
<img width="40%" height="40%" alt="Wiring" src="https://github.com/user-attachments/assets/775b6d97-a6ce-4606-afe1-18023c5d9f37" />
<br>
<br>

## WIFI & Web Server / API
The ESP32 connects to a WIFI network and hosts a http server with a webpage to control the projector screen, this webpage communicates with http requests which can also be used as an API.
<br>

### API - `http://0.0.0.0/`
`/up` - sends projector screen up <br>
`/down` - sends projector screen down <br>
`/lock` - locks the projector screens position (ignoring 5v projector signal and manual controls) <br>
Example of API being used in homeassistant.

### Setting / Changing Wifi SSID & Password
<strong>First Time Setup || Old Wifi Network Not Available: </strong> In the event that you are connecting the ESP to WIFI for the first time or the old WIFI Network is no longer available the ESP32 will wait 25 seconds of attempting to connect to the saved credentials, if all 50 connection attempts fail the ESP32 will host a WIFI Network called "Screen Controller Wifi Setup". Connect to this WIFI Network and proceed to 192.168.32.32 in your browser, from there click the WIFI Setup button and then fill in the details and press Save & Reboot. <br>

<strong>Changing WIFI Networks with OLD Wifi Network Available: </strong> In the event that you want to change the WIFI Network the ESP connects to when the old WIFI Network is still around, the ESP will connect to the old one. You have two choices either go to the webserver on the current WIFI Network and click the WIFI icon in the bottom right OR turn off the old network temporarily to allow the ESP to create its Hotspot.
