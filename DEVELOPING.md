# General instructions for developing
## Prerequisites
### Clone this repo
Guessing you've already done that.

### Install Arduino IDE
As stated in the main README, this project is using the Arduino IDE instead of VS Code/PlatformIO becasue I am very turned off of the way Copilot is being pushed into my face in VS Code.  Screw that.

*For the love of God, let's hope qualcomm doesn't fuck up Arduino and start adding AI bullshit.*

Start by going to https://docs.arduino.cc/software/ide/ to download the IDE.  **Don't use the cloud version - work locally**.  Get the installer for your flavor of OS and install it in the normal way.

### Get the needed board/library stuffs for the project
Start the Arduino IDE.  From the menu, select `Tools >> Boards >> Board Manager...`.  Be sure to select the "esp32 by Espressif" version, and click the `Install` button.  Once the board is selected, the IDE will download and install the compiler toolchain and standard libraries for the architecture.

Now for the libraries.  From the menu, select `Tools >> Manage Libraries...`  Using the Filter, search for and install the following four libraries (be sure to install the correct libraries, there are many with similar names to choose from):

+ ArduinoJson by Benoit (7.4.2)
+ EspSoftwareSerial by Dirk Karr, Peter Lerup (8.1.0)
+ LiteLED by Xylopyrographer (3.0.1)
+ NimBLE-Arduino by h2zero (2.3.7)

### Load project
From the Arduino menu, select `File >> Open...`.  Navigate to the project folder cloned from Git and open `./src/Flocker/Flocker.ino`  The main c++ file will open, along with all of the other support headers/source files in the source folder.

### Select and configure board
From the Arduino toolbar, click the `Select Board` drop down box, and click the `Select other board and port...` item.  Scroll through the list of boards and select `ESP32S3 Dev Module`. 

After choosing the board, select `Tools` from the Arduino menu.  The bottom half of the menu is used to configure the particular board in use.  Make the following changes to the items listed:
+ USB CDC on Boot -> change to `Enabled`.  This is needed to allow the built-in USB to serial converter and the command line interface
+ Flash -> change to `8MB`; this module has 8MB of flash instead of the normal 4Mb
+ Partition -> change to `Custom`.  There is a custom partition file in the project directory to set up how the 8MB flash is configured.  Generally, it's 4MB for application and 4MB for internal flash file system.
+ PSRAM -> change to `OPI PSRAM`.  The module has an extra 8MB of RAM available for use.  

## Test compile 
To test the installation and configuration, try to build the project.  From the Arduino menu, select `Sketch >> Verify/Compile` .  The `Output` window on the bottom of the IDE will show `Compiling sketch...` along with a progress bar.  This will take several minutes the first time, just let it run.
