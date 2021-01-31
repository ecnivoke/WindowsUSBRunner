# WindowsUSBRunner
 Runs a program on usb plugin

## Usage
 Simply create a file named 'WindowsAutoRun.bat' in the root directory of your USB drive.
 If WindowsUSBRunner.exe is active, it will detect the USB insert and run the batch file on your USB.

#### Commands
 --autorun <file> ***To define your own file to be ran, multiple files are allowed seperated by a comma.***
 --new-console ***Creates a new console with the application, only works if it's a console application.***

###### Example commands
WindowsUSBRunner.exe --autorun "run.bat","games\BloonsTD6.exe" --new-console
