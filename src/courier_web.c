#include "courier.h"

/* Browser bridge for "The Long Echo": the same C trip the story bench flies. */
static Courier courier;

void cw_reset(void);
void cw_tick(int frames, int command);
int cw_answer(int option);
double cw_value(int field);

void cw_reset(void) { courier_init(&courier, CE_ALL); }
void cw_tick(int frames, int command) { courier_tick(&courier, frames, command); }
int cw_answer(int option) { return courier_answer(&courier, option); }
double cw_value(int field) { return courier_value(&courier, field); }
