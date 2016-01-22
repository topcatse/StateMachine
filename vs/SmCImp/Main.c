#include <stdio.h>
#include <signal.h>
#include "StateMachineC.h"

#define HANDLED() StateMachine_handled(t->sm, SM_DUMMY)

typedef struct
{
	State s0;
	State s1;
    State s11;
    State s2;
    State s21;
    State s211;
    StateMachine sm;
    int foo;
} Tester;

typedef enum
{
    SM_A = SM_USER_START,
    SM_B,
    SM_C,
    SM_D,
    SM_E,
    SM_F,
    SM_G,
    SM_H,
	SM_X
} TesterSignals;

State Tester_s0(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e)
    {
        case SM_INIT:
            printf("S0-INIT ");
            StateMachine_initializer(t->sm, t->s1);
            return HANDLED();
        case SM_ENTRY:
            printf("S0-ENTRY ");
            return HANDLED();
        case SM_EXIT:
            printf("S0-EXIT ");
            return HANDLED();
        case SM_E:
            printf("S0-E ");
            StateMachine_transition(t->sm, t->s211);
            return HANDLED();
		case SM_X:
			printf("S0-X ");
			exit(0);
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
        case SM_INIT:
            printf("S1-INIT ");
            StateMachine_initializer(t->sm, t->s11);
            return HANDLED();
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
        case SM_B:
            printf("S1-B ");
            StateMachine_transition(t->sm, t->s11);
            return HANDLED();
        case SM_C:
            printf("S1-C ");
            StateMachine_transition(t->sm, t->s2);
            return HANDLED();
        case SM_D:
            printf("S1-D ");
            StateMachine_transition(t->sm, t->s0);
            return HANDLED();
        case SM_F:
            printf("S1-F ");
            StateMachine_transition(t->sm, t->s211);
            return HANDLED();
        default:
            break;
    }
    
    return t->s0;
}

State Tester_s11(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e)
    {
        case SM_ENTRY:
            printf("S11-ENTRY ");
            return HANDLED();
        case SM_EXIT:
            printf("S11-EXIT ");
            return HANDLED();
        case SM_G:
            printf("S11-G ");
            StateMachine_transition(t->sm, t->s211);
            return HANDLED();
        case SM_H:
            if (t->foo)
            {
                printf("S11-H ");
                t->foo = 0;
            }
            return HANDLED();
        default:
            break;
    }
    
    return t->s1;
}

State Tester_s2(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e)
    {
        case SM_INIT:
            printf("S2-INIT ");
            StateMachine_initializer(t->sm, t->s21);
            return HANDLED();
        case SM_ENTRY:
            printf("S2-ENTRY ");
            return HANDLED();
        case SM_EXIT:
            printf("S2-EXIT ");
            return HANDLED();
        case SM_C:
            printf("S2-C ");
            StateMachine_transition(t->sm, t->s1);
            return HANDLED();
        case SM_F:
            printf("S2-F ");
            StateMachine_transition(t->sm, t->s11);
            return HANDLED();
        default:
            break;
    }
    
    return t->s0;
}

State Tester_s21(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e)
    {
        case SM_INIT:
            printf("S21-INIT ");
            StateMachine_initializer(t->sm, t->s211);
            return HANDLED();
        case SM_ENTRY:
            printf("S21-ENTRY ");
            return HANDLED();
        case SM_EXIT:
            printf("S21-EXIT ");
            return HANDLED();
        case SM_B:
            printf("S21-B ");
            StateMachine_transition(t->sm, t->s211);
            return HANDLED();
        case SM_H:
            if (!t->foo)
            {
                printf("S21-H ");
                t->foo = 1;
            }
            return HANDLED();
        default:
            break;
    }
    
    return t->s2;
}

State Tester_s211(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e)
    {
        case SM_ENTRY:
            printf("S211-ENTRY ");
            return HANDLED();
        case SM_EXIT:
            printf("S211-EXIT ");
            return HANDLED();
        case SM_D:
            printf("S211-D ");
            StateMachine_transition(t->sm, t->s21);
            return HANDLED();
        case SM_G:
            printf("S211-G ");
            StateMachine_transition(t->sm, t->s0);
            return HANDLED();
        default:
            break;
    }
    
    return t->s21;
}

char* stateAsTxt(State s)
{
    if (s->stateFcn_ == Tester_s0)   return "S0";
    if (s->stateFcn_ == Tester_s1)   return "S1";
    if (s->stateFcn_ == Tester_s11)  return "S11";
    if (s->stateFcn_ == Tester_s2)   return "S2";
    if (s->stateFcn_ == Tester_s21)  return "S21";
    if (s->stateFcn_ == Tester_s211) return "S211";
    return "Nada";
}

Tester tester;

void interuptHandler(int sig)
{
	if (sig == SIGINT)
	{
		StateMachine_dispatch(tester.sm, SM_X);;
	}
}

int main(int argc, char* argv[])
{
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, interuptHandler);

	tester.s0   = State_ctor(&tester, Tester_s0);
	tester.s1   = State_ctor(&tester, Tester_s1);
    tester.s11  = State_ctor(&tester, Tester_s11);
    tester.s2   = State_ctor(&tester, Tester_s2);
    tester.s21  = State_ctor(&tester, Tester_s21);
    tester.s211 = State_ctor(&tester, Tester_s211);
    tester.sm = StateMachine_ctor();
    tester.foo = 0;
    
    StateMachine_open(tester.sm, &tester, tester.s0);
    
    for(;;)
    {
        printf("\n%s<-signal:", stateAsTxt(StateMachine_current(tester.sm)));
        
        char c = getc(stdin);
        getc(stdin); // discard '\n'
        
        StateMachine_dispatch(tester.sm, c - 'a' + SM_USER_START);
    }
    
	return 0;
}