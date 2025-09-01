# 3-Wire Projector Screen Controller
A project made with an ESP32 and a dual channel relay to replace a 3 way projector screen switch and instead automatically put the screen down when the USB port from the projector provides 5v power. Manual control is also possible VIA http webpage or http API. <br>
[View My Setup & Wiring](https://github.com/etahn-git/3-wire-projector-screen-controller/blob/f2d29850c5e23c0156d94c3bae05f29cf0711ab9/mySetup.md)
<br>
## Wiring Diagram & Photos
<img width="40%" height="40%" alt="Wiring" src="https://github.com/user-attachments/assets/775b6d97-a6ce-4606-afe1-18023c5d9f37" /><img width="40%" height="40%" alt="Screen Control" src="https://github.com/user-attachments/assets/685e2653-4428-46ac-94d2-cb9ad1360f8c" />

[More Photos](https://github.com/etahn-git/3-wire-projector-screen-controller/blob/c448231ddcf1ad4a557e2c4e193f5b26edf101fb/PHOTOS.md)

<br>

## WIFI & Web Server / API
The ESP32 connects to a WIFI network and hosts a http server with a webpage to control the projector screen, this webpage communicates with http requests which can also be used as an API.
<br>

### API - `http://0.0.0.0/`
`/up` - sends projector screen up <br>
`/down` - sends projector screen down <br>
`/lock` - locks the projector screens position (ignoring 5v projector signal and manual controls) <br>
`/toggle` - puts screen up / down <br>
Example of API being used in [Homeassistant](https://github.com/etahn-git/3-wire-projector-screen-controller/blob/224f235cc4e9872c08a74930ee3d80185485f388/HA.md).

### Setting / Changing Wifi SSID & Password
<strong>First Time Setup || Old Wifi Network Not Available: </strong> In the event that you are connecting the ESP to WIFI for the first time or the old WIFI Network is no longer available the ESP32 will wait 25 seconds of attempting to connect to the saved credentials, if all 50 connection attempts fail the ESP32 will host a WIFI Network called "Screen Controller Wifi Setup". Connect to this WIFI Network and proceed to http://192.168.32.32/ in your browser, from there click the WIFI Setup button and then fill in the details and press Save & Reboot. <br>

<strong>Changing WIFI Networks with OLD Wifi Network Available: </strong> In the event that you want to change the WIFI Network the ESP connects to when the old WIFI Network is still around, the ESP will connect to the old one. You have two choices either go to the webserver on the current WIFI Network and click the WIFI icon in the bottom right OR turn off the old network temporarily to allow the ESP to create its Hotspot.

<br>

## Configuration
* Variables `triggerPin`, `relayUp`, `relayDown` pins may need to be changed based off board manufacturer.
* `delay(10000);` this is the delay for how long a relay remains active before turning off when the screen goes down, may need to change based off length of screen.


## Mobile App
Create one using https://appsgeyser.com/ I use it and it works great
