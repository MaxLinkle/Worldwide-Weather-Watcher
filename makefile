all: compilation

df_cpu=16000000
mmcu=atmega328p
com=/dev/ttyACM1

Arduino_lib=/usr/share/arduino/hardware/arduino/cores/arduino
Arduino_pin=/usr/share/arduino/hardware/arduino/variants/standard

N_fichier=main
chemin=$(pwd)

Obj_c:=$(shell find $(Arduino_lib) -type f -name *.c)
Obj_c:=$(Obj_c:%.c=%.o)

%.o : %.c
	@avr-gcc -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=$(mmcu) -DF_CPU=$(df_cpu) -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -D__PROG_TYPES_COMPAT__ -I$(Arduino_lib) -I$(Arduino_pin) $< -o $(subst $(Arduino_lib),$(chemin),$@)

Obj_cpp:=$(shell find $(Arduino_lib) -type f -name *.cpp)
Obj_cpp:=$(Obj_cpp:%.cpp=%.o)

%.o : %.cpp
	@avr-g++ -c -g -Os -Wall -fno-exceptions -ffunction-sections -std=gnu++11 -fdata-sections -mmcu=$(mmcu) -DF_CPU=$(df_cpu) -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -D__PROG_TYPES_COMPAT__ -I$(Arduino_lib) -I$(Arduino_pin) $< -o $(subst $(Arduino_lib),$(chemin),$@)

clean:
	@rm -Rf $(chemin)/file
	@mkdir -p $(chemin)/file/lib/avr-libc

#init: clean
#@mkdir -p $(chemin)/file/lib/avr-libc
#@cp ./init/Blink.ino $(chemin)

inject_cpp: clean
	@echo '#include "Arduino.h"' > $(chemin)/file/$(N_fichier).cpp
	@cat $(chemin)/$(N_fichier).ino >> $(chemin)/file/$(N_fichier).cpp
	@cat $(Arduino_lib)/main.cpp >> $(chemin)/file/$(N_fichier).cpp


compilation_obj: inject_cpp
	@avr-g++ -x c++  -MMD -c -mmcu=$(mmcu) -Wall -DF_CPU=$(df_cpu) -DARDUINO=160 -DARDUINO_ARCH_AVR -D__PROG_TYPES_COMPAT__ -I$(Arduino_lib) -I$(Arduino_pin) -Wall  -Os  $(chemin)/file/$(N_fichier).cpp -o $(chemin)/file/$(N_fichier).o

lib_c:
	@make chemin=$(chemin)/file/lib $(Obj_c)

lib_cpp:
	@make chemin=$(chemin)/file/lib $(Obj_cpp)

archivage: lib_c lib_cpp
	@avr-ar rcs $(chemin)/file/libcore.a $(subst $(Arduino_lib),$(chemin)/file/lib,$(Obj_c)) $(subst $(Arduino_lib),$(chemin)/file/lib,$(Obj_cpp))

link: compilation_obj archivage
	@avr-gcc -mmcu=$(mmcu) -Wl,-gc-sections -Os -o $(chemin)/file/$(N_fichier).elf $(chemin)/file/$(N_fichier).o $(chemin)/file/libcore.a -lc -lm

compilation: link
	@avr-objcopy -O ihex -R .eeprom $(chemin)/file/$(N_fichier).elf $(chemin)/file/$(N_fichier).hex
	@echo "compilation terminé"

televersement: compilation
	@avr-size $(chemin)/file/$(N_fichier).elf
	@avrdude -p $(mmcu) -c arduino -P $(com) -b115200 -U flash:w:$(chemin)/file/$(N_fichier).hex

#verif_size: compilation
#	@avr-size $(chemin)/file/$(N_fichier).elf
