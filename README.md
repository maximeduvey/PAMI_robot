This project is to manage the code and structure of the litle robot called palmi



### Libraries ###

# /libs/bcm2835-1.71  compile but you can't make make check, you need a "gpio" user group and add your user to it
sudo groupadd gpio
sudo usermod -aG gpio $USER
# after that it's a good pratice to have dedicated gpio folder 
sudo mkdir /dev/gpiomen
sudo mkdir /dev/gpiochip
# set owner
sudo chown root:gpio /dev/gpiomem
sudo chmod g+rw /dev/gpiomem
sudo chown root:gpio /dev/gpiochip*
sudo chmod g+rw /dev/gpiochip*
# (not tested) if you want to have permission and group persisten accross reboots, you can create a udev rule
sudo nano /etc/udev/rules.d/99-gpio.rules
# add to the file
SUBSYSTEM=="gpio", KERNEL=="gpiochip*", ACTION=="add", GROUP="gpio", MODE="0660"
SUBSYSTEM=="gpio", KERNEL=="gpio*", ACTION=="add", GROUP="gpio", MODE="0660"
SUBSYSTEM=="gpio", KERNEL=="gpiomem", ACTION=="add", GROUP="gpio", MODE="0660"
# reboot

