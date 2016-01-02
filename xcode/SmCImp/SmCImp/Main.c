#include <stdio.h>

#define SM_NTRACE 0
#include "StateMachineC.h"

#define HANDLED() StateMachine_handled(t->sm, SM_DUMMY)

typedef struct
{
	State s0;
	State s1;
    StateMachine sm;
} Tester;

typedef enum
{
    SM_A = SM_USER_START,
    SM_B,
    SM_C,
    SM_D,
    SM_E,
    SM_F,
    SM_G
} TesterSignals;

State Tester_s0(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e)
    {
        case SM_INIT:
            StateMachine_initializer(t->sm, t->s1);
            return HANDLED();
        case SM_ENTRY:
            printf("S0-ENTRY ");
            return HANDLED();
        case SM_EXIT:
            printf("S0-EXIT ");
            return HANDLED();
        default:
            break;
    }
    
    return StateMachine_topState(t, SM_DUMMY);
}

State Tester_s1(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e)
    {
        case SM_ENTRY:
            printf("S1-ENTRY ");
            return HANDLED();
        case SM_EXIT:
            printf("S1-EXIT ");
            return HANDLED();
        case SM_A:
            printf("S1-A ");
            StateMachine_transition(t->sm, t->s1);
            return HANDLED();
        case SM_D:
            printf("S1-D ");
            StateMachine_transition(t->sm, t->s0);
            return HANDLED();
        default:
            break;
    }
    
    return t->s0;
}

int main(int argc, char* argv[])
{
	Tester tester;

	tester.s0 = State_ctor(&tester, Tester_s0);
	tester.s1 = State_ctor(&tester, Tester_s1);
    tester.sm = StateMachine_ctor();
    
    StateMachine_open(tester.sm, &tester, tester.s0);
    
    for(;;)
    {
        printf("\nSignal<-");
        
        char c = getc(stdin);
        getc(stdin); // discard '\n'
        
        if (c < 'a' || 'h' < c)
        {
            return 0;
        }
        
        StateMachine_dispatch(tester.sm, c - 'a' + SM_USER_START);
    }
    
	return 0;
}