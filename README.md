# TemperatureSensor_DS1621

![Test Setup](/html/Testaufbau.png)

This is the main project directory.

You can open the project [ TemperatureSensor_DS1621.coproj] (TemperatureSensor_DS1621.coproj) with CooCox's CoIDE. Please let me know if you have any problems with that.

For that project I used the ARM Cortex-M3 Microcontroller (NXP LPC1769) and of course the DS1621 temperature sensor.

Discription:
With the software, a temperature measurement with the sensor DS1621 can be performed. 
The communication is realized using the I2C bus and the current temperature will be displayed on a LCD display.
Finally the current value of the temperature will be sent to the CAN bus using the identifier 0x854.

Open [index.html] (/html/index.html) in your browser to see the Doxygen documentation.

Please note: The comments in the code are in German, just message me if you have a question.

************************************************************************

# License

See the [LICENSE](LICENSE) file for license rights and limitations (MIT).

