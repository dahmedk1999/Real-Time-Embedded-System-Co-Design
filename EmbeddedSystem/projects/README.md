# This folder contains sample projects that you can build

All the labs are based on materials from [SJSU CMPE-146 - Professor Preetpal Kang](http://books.socialledge.com/books/embedded-drivers-real-time-operating-systems)

## Lab2: GPIO Driver 
![image](https://user-images.githubusercontent.com/38081550/95832812-ac28bc80-0cef-11eb-9eea-3b6b61dd5132.png)

### OBJECTIVE
1. Manipulate microcontroller's registers in order to access and control physical pins.
2. Use implemented driver to sense input signals and control LEDs.
3. Use FreeRTOS binary semaphore to signal between tasks.

* L3-Driver       [GPIO Header](lab2/l3_drivers/gpio_lab.h).

* L3-Driver       [GPIO Source](lab2/l3_drivers/sources/gpio_lab.c).

* L5-Application  [main](lab2/l5_application/main.c)
```
# Compile Command:
scons --project=lab2

# SJ2 Board Flash Command:
python nxp-programmer/flash.py --input _build_lab2/lab2.bin
```


## Lab3: GPIO Interrupt - Dynamic User Defined ISR Callback Driver
![image](https://user-images.githubusercontent.com/38081550/95846327-27926a00-0d00-11eb-9556-bc060a716a56.png)
![Alt text](http://books.socialledge.com/uploads/images/gallery/2019-09-Sep/isr.gif)

### OBJECTIVE
1. Semaphores
    * Wait on [Semaphore Design pattern](http://books.socialledge.com/books/embedded-drivers-real-time-operating-systems/page/binary-semaphores#bkmrk-design-pattern)
2. [Lookup table](http://books.socialledge.com/books/embedded-drivers-real-time-operating-systems/page/lookup-tables) structures and Function Pointers
    * code will allow the user to register their callbacks
    * Be sure to understand how [function pointers](http://books.socialledge.com/books/useful-knowledge/page/function-pointer) work
3. [Interrupts](http://books.socialledge.com/books/embedded-drivers-real-time-operating-systems/page/nested-vector-interrupt-controller-%28nvic%29)
    * LPC supports rising and falling edge interrupts on certain port pins
    * These port/pin interrupts are actually OR'd together and use a single CPU interrupt.
    * On the SJ2 board, GPIO interrupts are handled by a dedicated GPIO interrupt (exception number 54)
    
* L3-Driver       [GPIO ISR Header](lab3/l3_drivers/gpio_isr.h).

* L3-Driver       [GPIO ISR Source](lab3/l3_drivers/sources/gpio_isr.c).

* L5-Application  [main](lab3/l5_application/main.c)
```
# Compile Command:
scons --project=lab3

# SJ2 Board Flash Command:
python nxp-programmer/flash.py --input _build_lab3/lab3.bin
```


## Lab4: ADC Driver + PWM Driver + FreeRTOS Queue
![image](https://user-images.githubusercontent.com/38081550/95839486-ed24cf00-0cf7-11eb-86e2-49fced46a6e1.png)

### OBJECTIVE
Improve an ADC driver, and use an existing PWM driver to design and implement an embedded application, which uses RTOS queues to communicate between tasks.
1. ADC Driver
   * Develop the driver functionality
   * Use a potentiometer that controls the analog voltage feeding into an analog pin of microcontroller
2. PWM Driver
   * Develop PWM Driver to control a GPIO 
   * An led brightness will be controlled, then create multiple colors using an RGB LED
3. FreeRTOS Tasks
   * Use FreeRTOS queues to communicate between ADC_Task and PWM_Task  
* L3-Driver       [ADC Header](lab4/l3_drivers/adc.h)     [PWM Header](lab4/l3_drivers/pwm1.h).

* L3-Driver       [ADC Source](lab4/l3_drivers/sources/adc.c)     [PWM Source](lab4/l3_drivers/sources/pwm1.c).

* L5-Application  [main](lab4/l5_application/main.c)
```
# Compile Command:
scons --project=lab4

# SJ2 Board Flash Command:
python nxp-programmer/flash.py --input _build_lab4/lab4.bin
```

## Lab5: SPI Flash Interface
![image](https://user-images.githubusercontent.com/38081550/95846493-54df1800-0d00-11eb-9b6e-521936e632cd.png)
![image](https://user-images.githubusercontent.com/38081550/95843009-134c6e00-0cfc-11eb-8725-93fad87deb26.png)

### OBJECTIVE
The objective is to learn how to create a thread-safe driver for Synchronous Serial Port and to communicate with an external SPI Flash device.
1. SPI Driver
   * Develop the driver functionality
   * Develop SPI Flash Interface to be able to read and write a "page" of the SPI [flash memory](https://www.adestotech.com/wp-content/uploads/DS-AT25SF041_044.pdf)
2. Mutex Thread-Safe example
   * attempt to read Adesto flash manufacturer ID in two tasks simultaneously using Mutex
 
* L3-Driver       [SPI Header](lab5/l3_drivers/ssp2_lab.h)     

* L3-Driver       [SPI Source](lab5/l3_drivers/sources/ssp2_lab.c)     
* L5-Application  [main](lab5/l5_application/main.c)
```
# Compile Command:
scons --project=lab5

# SJ2 Board Flash Command:
python nxp-programmer/flash.py --input _build_lab5/lab5.bin
```


## Lab6: Universal Asynchronous Receiver-Transmitter (UART)  
![image](https://user-images.githubusercontent.com/38081550/95844496-d41f1c80-0cfd-11eb-80f7-2fb2d33b2d91.png)
![Alt text](http://www.openrtos.net/queue_animation.gif)

### OBJECTIVE
The objective is to learn how to create a thread-safe driver for Synchronous Serial Port and to communicate with an external SPI Flash device.
1. UART Driver
   * Develop the driver functionality.
   * Learn how to communicate between two devices using UART.
2. Reinforce old knowledges:
   * Interrupts by setting up an interrupt on [receive](http://books.socialledge.com/link/90#bkmrk-to-learn-how-to-comm).
   * FreeRTOS Queue and String manipulation. 
 
* L3-Driver       [UART Header](lab6/l3_drivers/uart_lab.h)     

* L3-Driver       [UART Source](lab6/l3_drivers/sources/uart_lab.c)     
* L5-Application  [main](lab6/l5_application/main.c)
```
# Compile Command:
scons --project=lab6

# SJ2 Board Flash Command:
python nxp-programmer/flash.py --input _build_lab6/lab6.bin
```


## Lab7: FreeRTOS Producer Consumer Tasks (Cooperative Context Switch)
![image](https://user-images.githubusercontent.com/38081550/95926339-e1292380-0d70-11eb-964f-f89f12682c9c.png)

### OBJECTIVE
Queues' API can also perform context switches, but this is a type of Cooperative Context Switch
1. How Tasks and Queues work
   * xQueueSend()
   * xQueueReceive()
2. Assess how task priorities affect the RTOS Queue cooperative scheduling.
3. An extra part on how to create [CLI command](http://books.socialledge.com/books/embedded-drivers-real-time-operating-systems/page/sj2-board#bkmrk-cli-commands) to control the Task
     
* L5-Application  [main](lab7/l5_application/main.c)
```
# Compile Command:
scons --project=lab7

# SJ2 Board Flash Command:
python nxp-programmer/flash.py --input _build_lab7/lab7.bin
```


## lpc40xx_freertos
* This the primary LPC40xx project with all of the sources
* This is the default project that gets compiled when you simply type `scons` without any arguments
Build the project by using any of these commands:
```
# scons
scons --project=lpc40xx_freertos

# Build without any printf/scanf of float (saves program flash space, and is faster to program)
scons --project=lpc40xx_freertos --no-float-format
# Build without unit tests
scons --project=lpc40xx_freertos --no-float-format --no-unit-test
# Build with multiple threads on your machine
scons --project=lpc40xx_freertos --no-float-format --no-unit-test -j32

```











## x86_freertos
This is the FreeRTOS "simulator" on your host machine. For example, you can run a program with multiple FreeRTOS tasks on your Mac or Linux machine (or maybe even windows?)

* Learn FreeRTOS API
* Experiment with multiple FreeRTOS tasks or queues
```
# Compile
scons --project=x86_freertos

# Run the executable
_build_x86_freertos/./x86_freertos.exe
```

## x86_sandbox
This is to compile a program on your host machine. For example, you can compile an executable that runs on your Mac or Linux machine (or maybe even windows?)
```
# Compile
scons --project=x86_sandbox

# Run the executable
_build_x86_sandbox/./x86_sandbox.exe
```

Use this project to:
* Compile a program for your host machine
* Run unit-tests for code modules
