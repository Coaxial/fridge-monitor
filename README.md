fridge-monitor
==============

Monitors fridge + freezer temps

I wrote this Arduino sketch to monitor the temperature of the freezer and the fridge. It shows the current temps on a 4*20 HD4470 LCD along with the trend for each temperature sensor.

It has an alarm for when the temps are too high or too low.

Current issues:
  - Can't get reliable readings from the temperature sensors
  - Uses way too much energy to be run on batteries
