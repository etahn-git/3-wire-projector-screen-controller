
# Using the API in Homeassistant

In HA configuration.yaml add this code: <br>
```
shell_command:
    screen_toggle: curl -X POST http://192.168.0.32/toggle
    screen_lock: curl -X POST http://192.168.0.32/lock
    screen_up: curl -X POST http://192.168.0.32/up
    screen_down: curl -X POST http://192.168.0.32/down
```
Then create a button with a Tap Action with the Action "perform-action" and the perform-action `"shell_command.screen_up" OR "shell_command.screen_down" OR "shell_command.screen_toggle" OR "shell_command.screen_lock"`

Example:
```
show_name: true
show_icon: true
type: button
entity: zone.home
icon: mdi:arrow-up
name: Screen Up
tap_action:
  action: perform-action
  perform_action: shell_command.screen_up
  target: {}
```
