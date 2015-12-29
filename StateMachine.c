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
#include <malloc.h>

void State_ctor( State self, StateFcn stateFcn )
{
	self->owner_ = 0;
	self->stateFcn_ = stateFcn;		
}

void State_dtor( State* self )
{
}

/// Initializer. Object takes ownership of load.
void State_init( State self, OWNER owner, StateFcn stateFcn )
{
	self->owner_ = owner;
	self->stateFcn_ = stateFcn;
}
	
void State_copyCtor( State self, State const other )
{
	self->owner_ = other->owner_;
	self->stateFcn_ = other->stateFcn_;		
}		

/// Utility swap method.
void State_swap( State self, State other )
{
	OWNER* tmp1 = self->owner_;
	self->owner_  = other->owner_;
	other->owner_ = tmp1;

	StateFcn tmp2 = self->stateFcn_;
	self->stateFcn_  = other->stateFcn_;
	other->stateFcn_ = tmp2;
}
	
/// Assignment operator.
//State State_assign( State self, State const other )
//{
//	State tmp;
//	State_copyCtor( &tmp, other );
//	State_swap( self, &tmp );
//	return self;
//}

/// Conversion operator to StateFcn.
StateFcn State_stateFcn( State self )
{
	return self->stateFcn_;
}

/// Equality operator.
int State_isEqual( State self, State const rhs )
{
	return self->stateFcn_ == rhs->stateFcn_;
}

/// Inequality operator.
int State_isNotEqual( State self, State const rhs )
{
	return self->stateFcn_ != rhs->stateFcn_;
}

/// Invoke transition in owner.
State State_invoke( State self, Signal const e )
{
	return self->stateFcn_( self->owner_, e );
}

//==============================================================================

/// Dispatch event.
int StateMachine_dispatch(StateMachine self, Signal e);

/// Call when there is a default initialization stateFcn.
void StateMachine_initializer(StateMachine self, State const s)
{
	StateMachine_current(self, s);
}

/// Call when a transition shall occur.
void StateMachine_transition(StateMachine self, State const s)
{
	StateMachine_target(self, s);
}

/// To be returned when there is no parent stateFcn.
State StateMachine_topState(StateMachine self, Signal const e)
{
	return 0;
}

/// Shall be called when an event has been accepted.
State StateMachine_handled(StateMachine self, Signal const e)
{
	return 0;
}

/// Current stateFcn mutator.
static void StateMachine_current(StateMachine self, State const current);

/// Pitcher stateFcn accessor.
static State StateMachine_pitcher(StateMachine self);

/// Target stateFcn accessor.
static State StateMachine_target(StateMachine self);

/// Pitcher stateFcn mutator.
static void StateMachine_pitcher(StateMachine self, State const pitcher);

/// Target stateFcn mutator.
static void StateMachine_target(StateMachine self, State const target);

/// Invoke init event in given stateFcn and init/entry events in all
/// possibly subsequent states. 
/// If init returns false, a self transition is invoked.
static void StateMachine_init(StateMachine self, State const* stateFcn);

/// Trace down to stateFcn that possibly handles the given signal.
/// Releases the event.
static int StateMachine_findPitcher(StateMachine self, Signal* e);

/// Invoke exit on all states from current stateFcn down to
/// pitcher stateFcn.
static void StateMachine_exitDownToPitcher(StateMachine self, );

/// Invoke entry event in given path.
/// path: all states between pitcher and target..
/// path: the states in the path might be updated but
/// not the path itself.
static void StateMachine_retraceEntryPath(StateMachine self, Deque path);


#if CART
template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::inquireEvent_ = SmEvent< T >( INQUIRE );

template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::initEvent_    = SmEvent< T >( INIT );

template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::entryEvent_   = SmEvent< T >( ENTRY );

template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::exitEvent_    = SmEvent< T >( EXIT );
#endif
//------------------------------------------------------------------------------

int StateMachine_open(StateMachine    self,
					  OWNER*          owner,
					  State const*    initial,
					  Signal const*   e)
{
   SM_TRACE( "StateMachine open" );

   self->owner_   = owner;
   self->pitcher_ = StateMachine_topState(self, DUMMY);
   self->current_ = StateMachine_topState(self, DUMMY);
   self->target_  = initial;
   
   State target = StateMachine_target(self);
   State_invoke(&target, ENTRY);

   StateMachine_init(self, target);
}

//------------------------------------------------------------------------------

int StateMachine_isInState(StateMachine self, State const* stateFcn)
{
   SM_TRACE( "StateMachine isInState" );

   if (StateMachine_current(self) == stateFcn )
   {
      return 2;
   }

   // and all states down to (not including) top.

   State next     = State(StateMachine_current(self), INQUIRE);
   State topState = StateMachine_topState(self->owner_);

   while ( next != topState )
   {
      if ( next == stateFcn )
      {
         return 1;
      }
      next = State_invoke( next, INQUIRE );
   }

   return 0;
}

//------------------------------------------------------------------------------

State StateMachine_current(StateMachine self)
{
   SM_TRACE( "StateMachine current" );
   return self->current_;
}

//------------------------------------------------------------------------------

static void StateMachine_current(StateMachine self, State const current)
{
   SM_TRACE( "StateMachine current( StateFcn current )" );
   self->current_ = current;
}

//------------------------------------------------------------------------------

static State StateMachine_pitcher(StateMachine self)
{
   SM_TRACE( "StateMachine pitcher" );
   return self->pitcher_;
}

//------------------------------------------------------------------------------

static void StateMachine_pitcher(StateMachine self, State const pitcher)
{
   SM_TRACE( "StateMachine pitcher( StateFcn pitcher )" );
   self->pitcher_ = pitcher;
}

//------------------------------------------------------------------------------

static void StateMachine_target(StateMachine self, State const target)
{
   SM_TRACE( "StateMachine target( StateFcn stateFcn )" );
   self->target_ = target;
}

//------------------------------------------------------------------------------

static State StateMachine_target(StateMachine self)
{
   SM_TRACE( "StateMachine< OWNER, T >::target" );
   return self->target_;
}

//------------------------------------------------------------------------------

int StateMachine_dispatch(StateMachine self, Signal e)
{
   SM_TRACE( "StateMachine::dispatch" );

   assert( e && "Bad event to StateMachine::dispatch" );

   // Used to elaborate internal transition.
   State target = StateMachine_target(self);
   State_invoke(&target, StateMachine_topState(self, DUMMY));

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
#define INVOKE(state, event) State_invoke(StateMachine_##state(self), event)
#define PITCHER() StateMachine_pitcher(self)
#define TARGET() StateMachine_target(self)

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

//------------------------------------------------------------------------------

template< class OWNER, class T >
bool
StateMachine< OWNER, T >::findPitcher( UserEvent* e )
{
   SM_TRACE( "StateMachine< OWNER, T >::findPitcher" );

   pitcher( current() );

   State< OWNER, T > next;

   while ( ( next = pitcher()( e ) ) != owner_->handled() && 
           pitcher() != owner_->topState() )
   {
      pitcher( next );
   }

   return ( pitcher() != owner_->topState() );
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::exitDownToPitcher()
{
   SM_TRACE( "StateMachine< OWNER, T >::exitDownToPitcher" );
   assert( pitcher() != owner_->topState() && "exitDownToPitcher" );

   State< OWNER, T > next( current() );

   while ( next != pitcher() )
   {
      assert( next );

      State< OWNER, T > tmp = next( &exitEvent_ );

      if ( tmp == owner_->handled() )
      {
         // EXIT handled, elicit parent.
         next = next( &inquireEvent_ );
      }
      else
      {
         // EXIT not handled (tmp = parent).
         next = tmp;
      }
   }
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::retraceEntryPath( Path& trace )
{
   SM_TRACE( "StateMachine< OWNER, T >::retraceEntryPath" );

   typename Path::const_iterator iter;
   typename Path::const_iterator end( trace.end() );

   for ( iter = trace.begin(); iter != end; ++iter )
   {
       State< OWNER, T > sw = *iter;
       sw( &entryEvent_ );
   }
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::init( State< OWNER, T > const& stateFcn )
{
   SM_TRACE( "StateMachine< OWNER, T >::init" );

   current( stateFcn );

   State< OWNER, T > next( stateFcn );

   while ( next( &initEvent_ ) == owner_->handled() )
   {
      // INIT was handled so current has been modified (by initEvent_ call).
      next = current();
      next( &entryEvent_ );
   }
}

#endif