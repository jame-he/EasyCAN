Easy and cheap CAN interface
============================

This repo contains a project of a CAN interface. Main goals are:
- should be cheap
- one layer for homemade PCB or PCB manufacturing in 5cm\*5cm design
- flexible (galvanic isolated is an option)
- useable by many programs and OS (SLCAN API)
- no PIC programmer needed

Price target is 15$ - depends on galvanic isolation 

![alt text](https://github.com/GBert/EasyCAN/blob/master/pictures/easy_can_board_front.jpg "PCB front")
![alt text](https://github.com/GBert/EasyCAN/blob/master/pictures/easy_can_board_back.jpg "PCB back including CP2102")
![alt text](https://github.com/GBert/EasyCAN/blob/master/pictures/easy_can-test_setup.jpg "EasyCAN test setup")


### Firmware

Bootloader (ds30loader) is programmed by the cheap CP2102 board with Darron Broads [k8048](http://dev.kewl.org/k8048/Doc/). The
firmware itself is programmed by the bootloader. So no PIC programmer needed.

### Status

- working test PCB
- working ds30loader firmware
- working bin loader (modified pirate-loader for PIC18F26K80)
- wip: SLCAN firmware (early stage: send test CAN frames)

