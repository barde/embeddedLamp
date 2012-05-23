embeddedLamp
========

Kissig Embedded Systems SS 2012

Group members
--------------
* Kamil Wozniak
* Simon Wiesmann
* Mathias Gölke
* Bartholomäus Dedersen

Overview
--------

![General overview of project](https://github.com/Phialo/embeddedLamp/raw/master/overview.png) "Some rough overview in a simulated UML environment")

Connection Hint for Working at Home
---------------------------------

1. Open a Console
2. Connect either to rze-idefix1.rz.e-technik.fh-kiel.de per SSH or the VPN - [Instruction](http://www.fh-kiel.de/index.php?id=6225)
3. Connect to *149.222.55.222* on port 7788 as user panda, i.e.
`ssh -p 7788 panda@149.222.55.222`
4. Password is known for users of the project(see eMails)
5. You can now access the */sys/class/gpio* files and try out using the LED as blinking debug port

Kernel sources and compiler are on the system.

Documentation Snippets
-----------------------
`panda@panda:~/embeddedLamp/kernel-module$ zcat /proc/config.gz  | grep CONFIG_SPI
CONFIG_SPI=y`

two differnt kind of modes for SPI:
  Controller drivers ... controllers may be built in to System-On-Chip
	processors, and often support both Master and Slave roles.
	These drivers touch hardware registers and may use DMA.
	Or they can be PIO bitbangers, needing just GPIO pins.

  Protocol drivers ... these pass messages through the controller
	driver to communicate with a Slave or Master device on the
	other side of an SPI link.
from: http://www.kernel.org/doc/Documentation/spi/spi-summary

We use a protocol driver

first success picture:
![D2=Data, D1=Chip Select, D0=Clock](https://github.com/Phialo/embeddedLamp/raw/master/firstSuccess.png)
