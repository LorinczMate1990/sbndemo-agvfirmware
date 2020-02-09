#include "DigitalPorts.h"


void enableFallingEdgeInterrupt(DigitalInout *w){
    *(w->port->ifg) &= ~w->bit;  // A megszakításflag. A memóriaszemét miatt kell törölni.
    *(w->port->ie) |= w->bit;    // A P1IE jelentése Port1 Interrupt enable. Amelyik bit 1, azon a lábon engedélyezve van a megszakítás
    *(w->port->ies) &= ~w->bit;  // P1.2 Hi/lo falling edge. A megszakítások mindig élre váltódnak ki. Itt nekünk most a lefutó élre. (Amikor megnyomjuk a gombot, az a chip-ben lefutó él lesz)
}

void enableRisingEdgeInterrupt(DigitalInout *w){
    *(w->port->ies) |= w->bit;  // P1.2 Hi/lo falling edge. A megszakítások mindig élre váltódnak ki. Itt nekünk most a lefutó élre. (Amikor megnyomjuk a gombot, az a chip-ben lefutó él lesz)    
    *(w->port->ifg) &= ~w->bit;  // A megszakításflag. A memóriaszemét miatt kell törölni.
    *(w->port->ie) |= w->bit;    // A P1IE jelentése Port1 Interrupt enable. Amelyik bit 1, azon a lábon engedélyezve van a megszakítás
}

void enablePullup(DigitalInout *w){
    *(w->port->ren) |= w->bit;
    *(w->port->out) |= w->bit;
}

void enablePulldown(DigitalInout *w){
    *(w->port->ren) |= w->bit;
    *(w->port->out) &= ~w->bit;
}

void useAsOutput(DigitalInout *w){
    *(w->port->dir) |= w->bit;
}

void useAsInput(DigitalInout *w){
    *(w->port->dir) &= ~w->bit;    
}

void setOutputLow(DigitalInout *w){
    *(w->port->out) &= ~w->bit;
}
void setOutputHigh(DigitalInout *w){
    *(w->port->out) |= w->bit;
}
void toggleOutput(DigitalInout *w){
    if ((*(w->port->out)) & w->bit){
        *(w->port->out) &= ~w->bit;
    } else {
        *(w->port->out) |= w->bit;
    }
}
char testInput(DigitalInout *w){
    return (*(w->port->in)) & w->bit;
}

char testInterruptFlag(DigitalInout *w){
    return *(w->port->ifg) & w->bit;
}

void clearInterruptFlag(DigitalInout *w){
    *(w->port->ifg) &= ~(w->bit);
}