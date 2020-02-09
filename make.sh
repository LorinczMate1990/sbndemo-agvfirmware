#!/bin/bash

# Szükséges függvények
function get { echo $1; read a; echo $a > .tmp; }

# fordító paraméterei
COMPILE_PATH="./objects"; # A lefordítandó objektumok útvonala
OBJ_PATH="./precompiled"; # Az előre lefordított objektumok útvonala
BIN_PATH="./bin"; # Hová kerüljön a kész bináros?

if [ ! -d "$COMPILE_PATH" ]; then
  mkdir $COMPILE_PATH;
fi

if [ ! -d "$OBJ_PATH" ]; then
  mkdir $OBJ_PATH;
fi

if [ ! -d "$BIN_PATH" ]; then
  mkdir $BIN_PATH;
fi

# Project neve
NAME="amber";

# Fordító
CC="msp430-gcc";

# Programozással kapcsolatos beállítások
PROG_TYPE="rf2500"; # Programozó típusa:
CPU="msp430f2274";  # A célprocesszor
BOARD="EZ430RF";    # A board típusa

# ------------------------------------------------------------------------
# Ezután tilos módosítani
# ------------------------------------------------------------------------

rm $COMPILE_PATH/*.o;
rm $BIN_PATH/*.elf;

# Saját fordítási állományok hozzáadása
SOURCES="`ls *.c`";

# Header állományokból a flag-ek összeállítása
INCLUDES="";
for i in $HEADERS; do
  INCLUDES+=" -I$i";
done

# Makrókból flag-ek generálása
tmp="$MACROS";
MACROS="";
for i in $tmp; do
	MACROS+="-D$i "
done

# Flagek összefűzése
CFLAGS="-std=gnu99 $MACROS -mmcu=$CPU -O3 -Wall -g $INCLUDES"

# Forrásállományok lefordítása
for i in $SOURCES; do
  SOURCE="$i";
  OBJ="`echo $SOURCE | cut -d. -f1`.o";
  #if [ $SOURCE -nt $COMPILE_PATH/$OBJ ]; # Mi van, ha egy header fájl módosult?
  #then
    echo "  !!! $i elkészítése folyamatban !!!";
    $CC $CFLAGS -c -o $COMPILE_PATH/$OBJ $SOURCE || exit 1
    echo "____________________________________"
  #fi
done

# A precompiled mappában vannak a szükséges objektumok, az objects-ben pedig a forrásállományokból fordítottak.
# Jöhet a linkelés

LINKER_OBJECTS="";
COMPILED="`ls $COMPILE_PATH/*.o`";

for i in $COMPILED; do
  LINKER_OBJECTS+="$i ";
done

# Most, hogy minden .o fájl benne van a LINKER_OBJECTS-ben, megkezdődhet a linkelés
echo "";
echo "  !!! Kezdődik a linkelés !!!";

$CC $CFLAGS -o $BIN_PATH/$NAME.elf $LINKER_OBJECTS || exit 1

echo "  !!! A fordítás befejeződött, a bináris előállt !!!";

# A fordítás megtörtént. a ./bin mappában előállt a *.elf kiterjesztésű bináris.
# Most azt kell ráprogramozni az eszközre.
BIN="./bin/$NAME.elf";
mspdebug $PROG_TYPE "prog $BIN"
