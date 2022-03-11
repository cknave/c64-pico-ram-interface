CARTCONV = cartconv
KICK_JAR = ${HOME}/opt/KickAssembler/KickAss.jar

.PHONY: all clean
.SUFFIXES: .asm .bin .crt

all: loader_rom.c loader_rom.h raspi.c raspi.h

loader_rom.c: loader_rom.bin
	python bin2c.py loader_rom.bin
loader_rom.h: loader_rom.c loader_rom.bin
	python bin2c.py loader_rom.bin

raspi.c: raspi.nuf
	python bin2c.py --skip 2 raspi.nuf
raspi.h: raspi.c raspi.nuf
	python bin2c.py --skip 2 raspi.nuf

.bin.crt:
	${CARTCONV} -p -n pico16k -t normal -i $< -o $@

.asm.bin:
	java -jar ${KICK_JAR} $< -vicesymbols

clean:
	rm -f loader_rom.c loader_rom.h loader_rom.bin loader_rom.crt loader_rom.sym loader_rom.vs
