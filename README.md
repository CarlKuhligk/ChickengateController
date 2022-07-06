# CickengateController
Contraption that opens or closes the gate of a chicken coop. It uses node red as brain and mqtt as comunication protocol. The gate will open and close depending on the position of the sun. You can define a time range for opening and closing so if the sunrise or sunset is outside of this range, the gate will be controlled by these times.

## Main components
- Linear Actuator with integrated rotary encoder
- ESP32 Matrix Core + Wifi antenna
- VNH3ASP30-E a full-bridge motor driver
- DC-DC Stepdown
- Waterproof Connectors TE Connectivity AMP Superseal 1.5
- 3D Printed and sealed Housing

<img src="https://user-images.githubusercontent.com/87583841/177508631-c604d70c-6840-4101-81b9-23fd38725103.png" width="1000">
<img src="https://user-images.githubusercontent.com/87583841/177508322-37b72492-7daa-4d70-b307-69d66dd07091.jpg" width="1000">

## Hardware dependencies
<img src="https://user-images.githubusercontent.com/87583841/177510831-5efa2f2f-81bf-4123-974b-35eee85741f6.png" width="1000">
* Current sense output of the motor driver is also filtered by a lowpass.

## NodeRed and Docker
<img src="https://user-images.githubusercontent.com/87583841/177511456-48f2e264-8f94-4337-a0d1-d63007fbd9b6.png" width="200">

### Required NodeRed Nodes
- 
