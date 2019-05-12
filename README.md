# ESE 350 Project

This project is maintained by [Kyler Mintah](https://github.com/kylermintah) & [Ransford Antwi](https://github.com/masterford)

# Overview
![Image](docs/overview.jpg)

We are building a miniature camera that serves as a wearable dietary documentation aid for patients with nutritional needs. The device uploads photos it takes to a webserver where the photos are filtered for food

# Blog
## Week 1 (4/8/19)
![Image](docs/20190421_135823.gif)

A large amount of our time was dominated by learning to use and configure our ESP32.

![Image](docs/makemenuconfig.jpeg)

We found ourselves embroiled in setting up low-level espressif software, OS environment variables, resourse paths and more in an effort to be able to build and flash our C/C++ code which was dependent on the [esp-idf libraries](https://github.com/espressif/esp-idf). After much effort & error, we managed to configure our laptops to interface with the ESP32 and run a simple "Hello World" script.

Up until this point we had been using an ArduCam which had all its support and libraries based on the arduino library. We spent a lot of time rewriting the Arduino libraries into our own libraries in pure C/C++ and while we feel we were close to acheiving this, the endeavour proved to take up too much time. Instead of reinventing the wheel, we decided to shift focus for week 2.

### Circuit End of Week 1
![Image](docs/IMG-20190409-WA0017.jpeg)



## Week 2 (4/15/19)

Taking a break from the camera, we  went on to learn how to use the UART Rx/Tx pins to interface a Razor IMU with the ESP32. After mapping our device pinout (as there seem to be a couple of ESP32 models out there with different pinouts) we were able to read orientation data straight from our IMU which will help us implement our 'deep sleep' mode based on device orientation.

![Image](docs/20190421_152900.gif)

We switched cameras from ArduCam to a much more adaptable OV7670 w/out FIFO and finally managed to process our image! This camera only has minimal dependencies on the Arduino library as opposed to the ArduCam. We are still awaiting the shipment of an ESP32 dedicated camera.

### Circuit End of Week 2
![Image](docs/april21circuit.jpg)


We setup a hotspot on a phone (as a user would) and connected our system to the network. We relayed the image to  a locally hosted webserver for now. We hope to pull the image for processing using dedicated machine learning libraries to determine whether the patient has captured an image of food or not.

![Image](docs/20190421_155116.jpg)

While we were able to successfully capture images using DMA, the images came out discoloured unless intense light was shone on the subject. We postulate this could be happening for a few reasons which we've narrowed down to the following:

- **Lack of FIFO:** from what we've read online, using the OV7670 without FIFO does not produce the best picture quality
- **Long Wires:** We have a very long path of wires until they finally reach their destination. They all could be acting as antennas. Shortening these connections might increase image quality.
- **Current Imbalance:** We might be using an illadvised pull-up resistor combination that is incorrectly balancing the output.
- **Damaged Module:** This camera has been circulating Detkin for quite some time.

In any case, we have ordered an ESP32 dedicated camera that hopefully we will be able to integrate soon.

## Week 3 (4/22/19)

![Image](docs/AltiumSplash.PNG)

Although we had been behind schedule we managed to design a multilayer PCB in Altium. 

![Image](docs/PCB123.PNG)

We did not have much time to review it as we were already behind schedule due to days of troubleshooting and writing our abandoned custom libraries to enable camera functionality. 
Had we had more time, we would have added a VCC and GND layer with vias leading to each to simplify our routing. We also would have liked to implement a more stringent design rule check. We felt our routes may have been too close together and too thin.

![Image](docs/SchematicSmaller.PNG)

**IMU Update**

![Image](docs/IMURazor.png)

We have now pivoted back to the IMU subsytem and are focussed on integrating it with the camera functionality. 

Unfortunately the integration was not a simple task. We eventually ran into problems with the ESP32 scheduler and the task watchdog timer. 
The default clock tick rate is 100Hz which is configurable but generally 100 Hz provides a good balance between ISR overhead and responsiveness.

![Image](docs/ErrorMessage.png)

The implication of this was that portTICK_PERIOD_MS was 10ms per tick. Hence a 2ms vTaskDelay corresponds a 0 tick delay, which means no delay at all, which is interpreted as "yield to higher priority tasks, otherwise keep running". Thus our io_task may be starving our lower priority tasks, such as the Idle task, if it never blocks waiting for something else to happen.

We read that to solve this we can try disabling the watchdog timer but we became weary of this as this would starve lower priority tasks like the idle task which is actually utilized for cleanup of deleted tasks. The FreeRTOS timer task also runs at low priority and hence may be perpetually blocked.

We are still currently exporing a fix to this.

![Image](docs/WatchdogTrigger.PNG)

## Week 4 (4/27/19)
We fixed our task scheduling errors by multithreading the system and running the camera and IMU task in parallel. This helped prevent the starvation of certain task. The PCB was completed in time for the baseline demo, however we were not able to generate any images from the camera, not even the discolored ones we produced earlier which was quite a shock. We were able to integrate our project with Google Cloud Services in order to send telemetry data as well as possibly store images on the cloud. We opted not to store the images on the cloud yet due to the fact that we would have to pay for cloud storage but it's certainly possible since we are already able to send telemetry data.

![Image](docs/googlecloud.png)



### Support or Contact

Having trouble reaching us? Check out our [website](https://www.silkblu.com) or [contact us](mailto:kmintah@seas.upenn.edu) and we’ll help you sort it out.
