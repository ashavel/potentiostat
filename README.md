This project is a part of article «Building potentiostat from scratch: simple yet precise Arduino-based platform for voltammetry, chronoamperometry and capacitance measurements», summited to Chemistry Education Research and Practice in January 2022.
The project includes two applications.
The first one is an Arduino IDE code named firmware_potentiostat.ino in the root of the current depository. It should be uploaded to an Arduino nano board with the corresponding (desribed in the article) analog schematic to become functional.
The second application is written in Python and is compiled for Windows and Linux operating systems. To find the Windows version, please go to releases (https://github.com/ashavel/potentiostat/releases) and download Potentiostat_01.0-alpha.zip archive, extract it into a specific folder and execute Potentiostat_0.1.0-alpha.exe.

Program operation:
Please make sure that the potentiostat, assembled according to the article instructions and with correctly loaded Arduino code, is connected to your PC before you start the application.
To start the program, tap «Connect» and select the COM port used by your Arduino.
Four tabs named LSV, CV, Amperometry and Capacitance allow operation in corresponding four modes. Settings are standard for classic potentiostat.

Some specific values include:

-	Shunt value.
The potentiostat Arduino code sends to the computer the voltage drop on the shunt resistor instead of real current value. To calculate current, this value is divided by the shunt resistor value that should be introduced in the PC program.

-	Void steps.
Number of readings being ignored (omitted) before the flow of data, corresponding to a specific branch of data, starts. Can be useful in case if omission of noisy first data is needed not only at startup, but also between executions of branches. For most cases, however, we recommend setting this value to zero and using clearly indicated «Wait before» option, adding waiting time at start-up only.

-	Wait before.
Time value, explicitly setting a period of time in seconds the system should wait polarized by starting potential before the execution of scan starts.

After selection of correct method settings, tap «Run» to start method execution.

Alternatively, old data can be open for analysis by tapping «Open».

To save results, tap «Save».
