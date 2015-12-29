#include "../StateMachineC.h"

typedef struct
{
	State state1;
	State state2;
} Tester;

State Tester_state1(OWNER* owner, Signal const* s)
{
	Tester* tester = owner;
	return tester->state2;
}

State Tester_state2(OWNER* owner, Signal const* s)
{
	Tester* tester = owner;
	return tester->state1;
}

int main(int argc, char* argv[])
{
	Tester tester;

	State_ctor(&tester.state1, Tester_state1);
	State_ctor(&tester.state2, Tester_state2);

	return 0;
}