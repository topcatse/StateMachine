//=============================================- -*- C -*- ===================
//
// File Name     StateMachine.c
// Author        Tommy Carlsson
//
// This file contains the implementation of StateMachine.
//
//==============================================================================

#include "StateMachineC.h"
#include <assert.h>
#include <stdlib.h>

State State_ctor( OWNER owner, StateFcn stateFcn )
{
   State state = malloc(sizeof(struct State));
   State_init(state, owner, stateFcn);
   return state;
}

void State_dtor( State self )
{
   free(self);
}

/// Initializer. Object takes ownership of load.
void State_init( State self, OWNER owner, StateFcn stateFcn )
{
	self->owner_    = owner;
	self->stateFcn_ = stateFcn;
}

/// Conversion operator to StateFcn.
StateFcn State_stateFcn( State self )
{
	return self->stateFcn_;
}

/// Equality operator.
bool State_isEqual( State self, State const rhs )
{
	return self->stateFcn_ == rhs->stateFcn_ &&
           self->owner_    == rhs->owner_;
}

/// Inequality operator.
bool State_isNotEqual( State self, State const rhs )
{
    return self->stateFcn_ != rhs->stateFcn_ ||
           self->owner_    != rhs->owner_;
}

/// Invoke transition in owner.
State State_invoke( State self, Signal e )
{
	return self->stateFcn_( self->owner_, e );
}

//==============================================================================

struct DequeNode_t
{
	State				state_;
	struct DequeNode_t* next_;
	struct DequeNode_t* prev_;
	bool				used_;
};

typedef struct DequeNode_t* DequeNode;

struct Deque_t
{
	DequeNode			head_;
	DequeNode			tail_;
	int					nbrOfItems_;
	struct DequeNode_t	nodes_[STATEMACHINE_MAX_DEPTH];
};

typedef struct Deque_t* Deque;

//------------------------------------------------------------------------------

static DequeNode Deque_acquireNode(Deque self)
{
	DequeNode node = NULL;
	int i = 0;
	for (i = 0; i != STATEMACHINE_MAX_DEPTH; i++)
	{
		node = &self->nodes_[i];
		if (!node->used_)
			break;
	}

	if (!node) return NULL;

	node->used_ = true;

	return node;
}

//------------------------------------------------------------------------------

static void Deque_releaseNode(Deque self, DequeNode node)
{
	node->used_ = false;
}

//------------------------------------------------------------------------------

static void Deque_init(Deque self)
{
	assert(self && "Deque instance");

	self->head_ = NULL;
	self->tail_ = NULL;
	self->nbrOfItems_ = 0;

	int i;
	for (i = 0; i < sizeof(self->nodes_) / sizeof(self->nodes_[0]); i++)
	{
		self->nodes_[i].state_ = NULL;
		self->nodes_[i].next_ = NULL;
		self->nodes_[i].prev_ = NULL;
		self->nodes_[i].used_ = false;
	}
}

//------------------------------------------------------------------------------

static bool Deque_push(Deque self, State state)
{
	assert(self);

	DequeNode node;

	node = Deque_acquireNode(self);
	if (!node)
	{
		return false;
	}

	node->next_ = self->tail_;
	node->prev_ = NULL;
	node->state_ = state;

	if (self->tail_)
	{
		self->tail_->prev_ = node;
	}

	if (!self->head_)
	{
		self->head_ = self->tail_;
	}

	self->tail_ = node;
	self->nbrOfItems_++;

	return true;
}

//------------------------------------------------------------------------------

static State Deque_pop(Deque self)
{
	assert(self);

	State state;
	if (!self->tail_)
	{
		return NULL;
	}

	DequeNode prevTail;
	prevTail = self->tail_;
	self->tail_ = prevTail->next_;
	if (self->tail_)
	{
		self->tail_->prev_ = NULL;
	}

	self->nbrOfItems_--;
	state = prevTail->state_;
	Deque_releaseNode(self, prevTail);

	return state;
}

//------------------------------------------------------------------------------

static void Deque_popn(Deque self, int count)
{
	while (count)
	{
		Deque_pop(self);
		count--;
	}
}

//------------------------------------------------------------------------------

static int Deque_contains(Deque self, State const state)
{
	int pos = 1;
	DequeNode tmp = self->tail_;
	while (tmp)
	{
		if (State_isEqual(tmp->state_, state))
		{
			return pos;
		}
		tmp = tmp->next_;
		pos++;
	}

	return 0;
}

//==============================================================================

/// Pitcher state accessor.
static State StateMachine_pitcher(StateMachine self);

/// Target state accessor.
static State StateMachine_target(StateMachine self);

/// Current state mutator.
static void StateMachine_setCurrent(StateMachine self, State const current);

/// Pitcher state mutator.
static void StateMachine_setPitcher(StateMachine self, State const pitcher);

/// Target state mutator.
static void StateMachine_setTarget(StateMachine self, State const target);

/// Invoke init event in given state and init/entry events in all
/// possibly subsequent states.
/// If init returns false, a self transition is invoked.
static void StateMachine_init(StateMachine self, State state);

/// Trace down to state that possibly handles the given signal.
/// Releases the event.
static bool StateMachine_findPitcher(StateMachine self, Signal e);

/// Invoke exit on all states from current state down to
/// pitcher state.
static void StateMachine_exitDownToPitcher(StateMachine self );

/// Invoke entry event in given path.
/// path: all states between pitcher and target..
/// path: the states in the path might be updated but
/// not the path itself.
static void StateMachine_retraceEntryPath(StateMachine self, Deque path);

static struct State topState;
static struct State handledState;

//==============================================================================

#define PITCHER() StateMachine_pitcher(self)
#define TARGET() StateMachine_target(self)
#define CURRENT() StateMachine_current(self)
#define EQUAL(state1, state2) State_isEqual(state1, state2)
#define NEQUAL(state1, state2) State_isNotEqual(state1, state2)
#define PUSH(t, s) do{ result = Deque_push(t, s); assert(result); }while(0)

//------------------------------------------------------------------------------

StateMachine StateMachine_ctor()
{
	StateMachine sm = malloc(sizeof(struct StateMachine_t));
	return sm;
}

//------------------------------------------------------------------------------

void StateMachine_dtor(StateMachine self)
{
    free( self );
}

//------------------------------------------------------------------------------

void StateMachine_open(StateMachine   self,
                       OWNER          owner,
                       State const    initial)
{
   SM_TRACE( "StateMachine_open" );

   topState.stateFcn_     = StateMachine_topState;
   topState.owner_        = owner;
   handledState.stateFcn_ = StateMachine_handled;
   handledState.owner_    = owner;
   
   self->owner_   = owner;
   self->pitcher_ = &topState;
   self->current_ = &handledState;
   self->target_  = initial;
    
   State target = TARGET();
   State_invoke( target, SM_ENTRY );

   StateMachine_init(self, target);
}

//------------------------------------------------------------------------------

int StateMachine_isInState(StateMachine self, State const state)
{
   SM_TRACE( "StateMachine_isInState" );

   if ( CURRENT() == state )
   {
      return 2;
   }

   // and all states down to (not including) top.

   State next = State_invoke( CURRENT(), SM_INQUIRE );

   while ( NEQUAL( next, &topState ) )
   {
      if ( EQUAL( next, state ) )
      {
         return 1;
      }
      next = State_invoke( next, SM_INQUIRE );
   }

   return 0;
}

//------------------------------------------------------------------------------

State StateMachine_current(StateMachine self)
{
   SM_TRACE( "StateMachine_current" );
   return self->current_;
}

//------------------------------------------------------------------------------

bool StateMachine_dispatch(StateMachine self, Signal e)
{
    SM_TRACE( "StateMachine_dispatch" );
    
    assert( e && "Bad event to StateMachine::dispatch" );
    
    // Used to elaborate internal transition.
    StateMachine_setTarget( self, &topState );
   
    if ( !StateMachine_findPitcher( self, e ) )
    {
        // Signal is not handled.
        SM_TRACE( "StateMachine no pitcher" );
        return false;
    }
    
    // ( h) Internal transition.
    // Side-effect: Call to findPitcher() above, which possibly,
    // sets a new target. If target is unchanged it is considered as an
    // internal transition so step out.
    if ( EQUAL( TARGET(), &topState ) )
    {
        SM_TRACE( "StateMachine handled case (h)" );
        return true;
    }
    
    StateMachine_exitDownToPitcher(self);
    
    // (a) Handle transition to self.
    if ( EQUAL( PITCHER(), TARGET() ) )
    {
        SM_TRACE( "StateMachine handled case (a)" );
        State_invoke( PITCHER(), SM_EXIT );
        State_invoke( TARGET(), SM_ENTRY );
        StateMachine_init( self, TARGET() );
        return true;
    }
    
    // (b) Handle pitcher == targets' parent.
    State targetParent = State_invoke( TARGET(), SM_INQUIRE);
    if ( EQUAL( PITCHER(), targetParent ) )
    {
        SM_TRACE( "StateMachine handled case (b)" );
        State_invoke( TARGET(), SM_ENTRY );
        StateMachine_init( self, TARGET() );
        return true;
    }
    
    // (c) Handle pitcher's parent == targets' parent.
    State pitcherParent = State_invoke( PITCHER(), SM_INQUIRE );
    if ( EQUAL( pitcherParent, targetParent ) )
    {
        SM_TRACE( "StateMachine handled case (c)" );
        State_invoke( PITCHER(), SM_EXIT );
        State_invoke( TARGET(), SM_ENTRY );
        StateMachine_init( self, TARGET() );
        return true;
    }
    
    // (d) Handle pitcher's parent == target.
    if ( EQUAL( pitcherParent, TARGET() ) )
    {
        SM_TRACE( "StateMachine handled case (d)" );
        State_invoke( PITCHER(), SM_EXIT );
        StateMachine_init( self, TARGET() );
        return true;
    }
    
    // The target state hierarchy needs to be recorded.
    bool result;
    struct Deque_t trace_instance;
    Deque trace = &trace_instance;
    Deque_init( trace );
    
    PUSH( trace, TARGET() );
    PUSH( trace, targetParent );
    
    // (e) Handle pitcher == target's parent parent ... hierarchy.
    State next = State_invoke( targetParent, SM_INQUIRE );
    while ( NEQUAL( next, &topState ) )
    {
        if ( EQUAL( next, PITCHER() ) )
        {
            SM_TRACE( "StateMachine handled case (e)" );
            StateMachine_retraceEntryPath( self, trace );
            StateMachine_init( self, TARGET() );
            return true;
        }
        PUSH( trace, next );
        next = State_invoke( next, SM_INQUIRE );
    }
    PUSH( trace, &topState );
    
    // The remaining cases impose EXIT of pitcher.
    State_invoke( PITCHER(), SM_EXIT );
    
    // (f) Handle pitcher's parent == target's parent parent ... hierarchy.
    unsigned int pos = Deque_contains( trace, pitcherParent );
    if ( pos > 0 )
    {
        // Found Least Base Ancestor @ pos.
        // Erase it and its ancestors because ENTRY on these is not correct.
        SM_TRACE( "StateMachine handled case (f)" );
        Deque_popn( trace, pos );
        StateMachine_retraceEntryPath( self, trace );
        StateMachine_init( self, TARGET() );
        return true;
    }
    
    // (g) Handle pitcher's parent parent ... hierarchy for each target.
    bool search = true;
    next = pitcherParent;
    while ( search )
    {
        pos = Deque_contains( trace, next );
        if ( pos > 0 )
        {
            // Found Least Base Ancestor @ pos.
            // Erase it and its ancestors because ENTRY on these
            // is not correct.
            SM_TRACE( "StateMachine handled case (g)" );
			Deque_popn( trace, pos );
            StateMachine_retraceEntryPath( self, trace );
            StateMachine_init( self, TARGET() );
            return true;
        }
        
        if ( NEQUAL( next, &topState ) )
        {
            State_invoke( next, SM_EXIT );
            next = State_invoke( next, SM_INQUIRE );
        }
        else
        {
            search = false;
        }
    }
    
    assert( false && "Impossible StateMachine transition case" );
    return false;
}

//------------------------------------------------------------------------------

/// Call when there is a default initialization state.
void StateMachine_initializer(StateMachine self, State const s)
{
    StateMachine_setCurrent(self, s);
}

//------------------------------------------------------------------------------

/// Call when a transition shall occur.
void StateMachine_transition(StateMachine self, State const s)
{
    StateMachine_setTarget(self, s);
}

//------------------------------------------------------------------------------

/// To be returned when there is no parent state.
State StateMachine_topState(OWNER owner, Signal e)
{
    return &topState;
}

//------------------------------------------------------------------------------

/// Shall be called when an event has been accepted.
State StateMachine_handled(OWNER owner, Signal e)
{
    return &handledState;
}

//------------------------------------------------------------------------------

static void StateMachine_setCurrent(StateMachine self, State const current)
{
   SM_TRACE( "StateMachine_setCurrent( State current )" );
   self->current_ = current;
}

//------------------------------------------------------------------------------

static State StateMachine_pitcher(StateMachine self)
{
   SM_TRACE( "StateMachine_pitcher" );
   return self->pitcher_;
}

//------------------------------------------------------------------------------

static void StateMachine_setPitcher(StateMachine self, State const pitcher)
{
   SM_TRACE( "StateMachine_setPitcher" );
   self->pitcher_ = pitcher;
}

//------------------------------------------------------------------------------

static void StateMachine_setTarget(StateMachine self, State const target)
{
   SM_TRACE( "StateMachine_setTarget" );
   self->target_ = target;
}

//------------------------------------------------------------------------------

static State StateMachine_target(StateMachine self)
{
   SM_TRACE( "StatStateMachine_target" );
   return self->target_;
}

//------------------------------------------------------------------------------

bool StateMachine_findPitcher( StateMachine self, Signal e )
{
   SM_TRACE( "StateMachine_findPitcher" );

   StateMachine_setPitcher( self, CURRENT() );

   State next = State_invoke( PITCHER(), e );

   while ( NEQUAL( next, &handledState ) && NEQUAL( PITCHER(), &topState ) )
   {
      StateMachine_setPitcher( self, next );
      next = State_invoke( PITCHER(), e );
   }

   return NEQUAL( PITCHER(), &topState );
}

//------------------------------------------------------------------------------

void StateMachine_exitDownToPitcher(StateMachine self)
{
   SM_TRACE( "StateMachine_exitDownToPitcher" );
   
   assert( NEQUAL( PITCHER(), &topState ) && "exitDownToPitcher" );

   State next = CURRENT();

   while ( NEQUAL( next, PITCHER() ) )
   {
      assert( next );

      State tmp = State_invoke( next, SM_EXIT );

      if ( EQUAL( tmp, &handledState ) )
      {
         // EXIT handled, elicit parent.
         next = State_invoke( next, SM_INQUIRE );
      }
      else
      {
         // No EXIT in this state (tmp = parent).
         next = tmp;
      }
   }
}

//------------------------------------------------------------------------------

void StateMachine_retraceEntryPath( StateMachine self, Deque trace )
{
    SM_TRACE( "StateMachine_retraceEntryPath" );

    State state = 0;
    while ( (state = Deque_pop( trace )) )
    {
        State_invoke( state, SM_ENTRY );
    }
}

//------------------------------------------------------------------------------

void StateMachine_init( StateMachine self, State state )
{
   SM_TRACE( "StateMachine_init" );

   StateMachine_setCurrent(self, state);

   State next = state;

   while ( State_invoke( next, SM_INIT ) == &handledState )
   {
      // INIT was handled so current has been modified (by initEvent_ call).
      next = CURRENT();
      State_invoke( next, SM_ENTRY );
   }
}

//==============================================================================
