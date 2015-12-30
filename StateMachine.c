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

State State_ctor( StateFcn stateFcn )
{
   State state = malloc(sizeof(struct State));
   State_init(state, 0, stateFcn);
   return state;
}

void State_dtor( State self )
{
   free(self);
}

/// Initializer. Object takes ownership of load.
void State_init( State self, OWNER owner, StateFcn stateFcn )
{
	self->owner_ = owner;
	self->stateFcn_ = stateFcn;
}

/// Conversion operator to StateFcn.
StateFcn State_stateFcn( State self )
{
	return self->stateFcn_;
}

/// Equality operator.
int State_isEqual( State self, State const rhs )
{
	return self->stateFcn_ == rhs->stateFcn_ &&
          self->owner_    == rhs->owner_;
}

/// Inequality operator.
int State_isNotEqual( State self, State const rhs )
{
   return self->stateFcn_ != rhs->stateFcn_ &&
          self->owner_    != rhs->owner_;
}

/// Invoke transition in owner.
State State_invoke( State self, Signal e )
{
	return self->stateFcn_( self->owner_, e );
}

//==============================================================================

/// Pitcher stateFcn accessor.
static State StateMachine_pitcher(StateMachine self);

/// Target stateFcn accessor.
static State StateMachine_target(StateMachine self);

/// Current stateFcn mutator.
static void StateMachine_setCurrent(StateMachine self, State const current);

/// Pitcher stateFcn mutator.
static void StateMachine_setPitcher(StateMachine self, State const pitcher);

/// Target stateFcn mutator.
static void StateMachine_setTarget(StateMachine self, State const target);

/// Invoke init event in given stateFcn and init/entry events in all
/// possibly subsequent states.
/// If init returns false, a self transition is invoked.
static void StateMachine_init(StateMachine self, State state);

/// Trace down to stateFcn that possibly handles the given signal.
/// Releases the event.
static int StateMachine_findPitcher(StateMachine self, Signal e);

/// Invoke exit on all states from current stateFcn down to
/// pitcher stateFcn.
static void StateMachine_exitDownToPitcher(StateMachine self, );

/// Invoke entry event in given path.
/// path: all states between pitcher and target..
/// path: the states in the path might be updated but
/// not the path itself.
static void StateMachine_retraceEntryPath(StateMachine self, Deque path);

static struct State topState;
static struct State handelState;

//===========================================================================

#define INVOKE(state, event) State_invoke(StateMachine_##state(self), event)
#define PITCHER() StateMachine_pitcher(self)
#define TARGET() StateMachine_target(self)
#define CURRENT() StateMachine_current(self)
#define EQUAL(state1, state2) State_isEqual(state1, state2)
#define NEQUAL(state1, state2) State_isNotEqual(state1, state2)

//------------------------------------------------------------------------------

void StateMachine_open(StateMachine   self,
					        OWNER          owner,
					        State const    initial,
					        Signal         e)
{
   SM_TRACE( "StateMachine_open" );

   topState.stateFcn_    = StateMachine_topState;
   topState.owner_       = owner;
   handelState.stateFcn_ = StateMachine_handled;
   handelState.owner_    = owner;
   
   self->owner_   = owner;
   self->pitcher_ = &topState;
   self->current_ = &handelState;
   self->target_  = initial;
    
   State target = TARGET();
   INVOKE(target, SM_ENTRY);

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

   State next = INVOKE( current, SM_INQUIRE );

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

int StateMachine_dispatch(StateMachine self, Signal e)
{
    SM_TRACE( "StateMachine_dispatch" );
    
    assert( e && "Bad event to StateMachine::dispatch" );
    
    // Used to elaborate internal transition.
    State target = StateMachine_target(self);
    State_invoke(target, StateMachine_topState(self, SM_DUMMY));
    
    if ( !StateMachine_findPitcher( self, e )
    {
        // Signal is not handled.
        SM_TRACE( "StateMachine no pitcher" );
        return false;
    }
    
    // ( h) Internal transition.
    // Side-effect: Call to findPitcher() above, which possibly,
    // sets a new target. If target is unchanged it is considered as an
    // internal transition so step out.
    if (StateMachine_target(self) == StateMachine_topState)
    {
        SM_TRACE( "StateMachine handled case (h)" );
        return true;
    }
    
    StateMachine_exitDownToPitcher(self);
    
    // (a) Handle transition to self.
    if (StateMachine_pitcher(self) == StateMachine_target(self) )
    {
        SM_TRACE( "StateMachine handled case (a)" );
        State_invoke(StateMachine_pitcher(self), EXIT);
        State_invoke(StateMachine_target(self), ENTRY);
        StateMachine_init(self, StateMachine_target(self) );
        return true;
    }
    
    // (b) Handle pitcher == targets' parent.
    State targetParent = INVOKE(target, INQUIRE);
    if (PITCHER() == targetParent )
    {
        SM_TRACE( "StateMachine handled case (b)" );
        INVOKE(target, ENTRY);
        StateMachine_init(self, TARGET());
        return true;
    }
    
    // (c) Handle pitcher's parent == targets' parent.
    State pitcherParent = INVOKE(pitcher, INQUIRE);
    if ( pitcherParent == targetParent )
    {
        SM_TRACE( "StateMachine handled case (c)" );
        INVOKE(pitcher, EXIT);
        INVOKE(target, ENTRY);
        StateMachine_init(self, TARGET());
        return true;
    }
    
    // (d) Handle pitcher's parent == target.
    if ( pitcherParent == target() )
    {
        SM_TRACE( "StateMachine handled case (d)" );
        INVOKE(pitcher, EXIT);
        StateMachine_init(self, TARGET());
        return true;
    }
    
    // The target stateFcn hierarchy needs to be recorded.
    struct deque_t trace_instance;
    Deque trace = &trace_instance;
    deque_init(trace, NULL);
    
    deque_append(trace, TARGET());
    deque_append(trace, targetParent);
    
    // (e) Handle pitcher == target's parent parent ... hierarchy.
    State next = State_invoke(targetParent, INQUIRE);
    while ( next != StateMachine_topState)
    {
        if ( next == PITCHER() )
        {
            SM_TRACE( "StateMachine handled case (e)" );
            StateMachine_retraceEntryPath(self, trace);
            StateMachine_init(self, TARGET());
            return true;
        }
        deque_append(trace, next);
        next = State_invoke(next, INQUIRE);
    }
    deque_append(trace, StateMachine_topState);
    
    // The remaining cases impose EXIT of pitcher.
    INVOKE(pitcher, EXIT);
    
    // (f) Handle pitcher's parent == target's parent parent ... hierarchy.
    typename Path::iterator pos;
    if ( ( pos = find( trace.begin(),
                      trace.end(),
                      pitcherParent ) ) != trace.end() )
    {
        // Found Least Base Ancestor @ pos.
        // Erase it and its ancestors because ENTRY on these is not correct.
        SM_TRACE( "StateMachine handled case (f)" );
        trace.erase( trace.begin(), ++pos );
        retraceEntryPath( trace );
        init( target() );
        return true;
    }
    
    // (g) Handle pitcher's parent parent ... hierarchy for each target.
    bool search(true);
    next = pitcherParent;
    while ( search )
    {
        if ( ( pos = find( trace.begin(),
                          trace.end(),
                          next ) ) != trace.end() )
        {
            // Found Least Base Ancestor @ pos.
            // Erase it and its ancestors because ENTRY on these
            // is not correct.
            SM_TRACE( "StateMachine handled case (g)" );
            trace.erase( trace.begin(), ++pos );
            retraceEntryPath( trace );
            init( target() );
            return true;
        }
        if ( next != owner_->topState() )
        {
            next( &exitEvent_ );
            next = next( &inquireEvent_ );
        }
        else
        {
            search = false;
        }
    }
    
    assert( false && "Impossible StateMachine transition case" );
    return false;
}

/// Call when there is a default initialization stateFcn.
void StateMachine_initializer(StateMachine self, State const s)
{
    StateMachine_setCurrent(self, s);
}

/// Call when a transition shall occur.
void StateMachine_transition(StateMachine self, State const s)
{
    StateMachine_setTarget(self, s);
}

/// To be returned when there is no parent stateFcn.
State StateMachine_topState(OWNER owner, Signal e)
{
    return &topState;
}

/// Shall be called when an event has been accepted.
State StateMachine_handled(OWNER owner, Signal e)
{
    return &handelState;
}

//------------------------------------------------------------------------------

static void StateMachine_setCurrent(StateMachine self, State const current)
{
   SM_TRACE( "StateMachine_setCurrent( StateFcn current )" );
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

int StateMachine_findPitcher( StateMachine self, Signal e )
{
   SM_TRACE( "StateMachine_findPitcher" );

   StateMachine_setPitcher( self, CURRENT() );

   State next = INVOKE( pitcher, e );

   while ( NEQUAL( next, &handelState ) && NEQUAL( PITCHER(), &topState ) )
   {
      StateMachine_setPitcher( self, next );
      next = INVOKE( pitcher, e );
   }

   return ( NEQUAL( PITCHER(), &topState ) );
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
         // EXIT not handled (tmp = parent).
         next = tmp;
      }
   }
}

//------------------------------------------------------------------------------

void StateMachine_retraceEntryPath( StateMachine self, Path& trace )
{
   SM_TRACE( "StateMachine_retraceEntryPath" );

   typename Path::const_iterator iter;
   typename Path::const_iterator end( trace.end() );

   for ( iter = trace.begin(); iter != end; ++iter )
   {
       State< OWNER, T > sw = *iter;
       sw( &entryEvent_ );
   }
}

//------------------------------------------------------------------------------

void StateMachine_init( StateMachine self, State state )
{
   SM_TRACE( "StateMachine_init" );

   StateMachine_setCurrent(self, state);

   State next = state;

   while ( next( &initEvent_ ) == owner_->handled() )
   {
      // INIT was handled so current has been modified (by initEvent_ call).
      next = current();
      next( &entryEvent_ );
   }
   
   State< OWNER, T > next( stateFcn );

   while ( next( &initEvent_ ) == owner_->handled() )
   {
      // INIT was handled so current has been modified (by initEvent_ call).
      next = current();
      next( &entryEvent_ );
   }
}

#endif