#ifndef DATAFUNCS_h
#define DATAFUNCS_h

extern void SetupTimer2();
ISR(TIMER2_OVF_vect); //Timer2 overflow interrupt vector handler
extern void assemble_dcc_msg();
extern void setData(unsigned char);


#endif
