#ifndef LIBDEBUG_H
#define LIBDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Other dependecy includes */

/* Prototype */
#ifdef DEBUG
extern	int		_DEBUGInit();
extern	void	_DEBUG(int, ...);
extern	void	_DEBUGEnd();
extern  void    _DEBUGChgLevel(int);
#endif
extern	void	DEBUGConsole(char * fmt, ...);

/* Costants */ 
#define DBG_FLAGS		0xFF00	/* Debuggging flags mask */
#define DBG_SIL			0x8000	/* Silent: don't print module_id and user_id */
#define DBG_TIME		0x4000	/* Print elapsed real time since DebugInit */
#define DBG_DATE		0x2000	/* Print date&time */
#define DBG_DATEONLY	0x1000	/* Print only date */
#define DBG_TIMEONLY	0x0800	/* Print only time */

#define	DBG_L_FATAL			 1
#define	DBG_L_NON_FATAL		 2
#define	DBG_L_START_F		 3
#define	DBG_L_STOP_F		 4
#define	DBG_L_TRACE			 5
#define	DBG_L_DUMP_V		 6
#define	DBG_L_DUMP_A		 7

/* Macroes */
/*
 * Conditional use of Debug.
 * To use Debug directly, call _DEBUG.
 *
 * WARNING: in the source code, Debug calls MUST have double parentheses
 *	    as:
 *	    Debug(( 9, "Errno = %d\n", errno ));
 *	    AND the first "(" MUST immediatly follow the "debug" word, thus:
 *
 *	    Debug(( 9, "Errno = %d\n", errno ));  -- CORRECT
 *	    Debug (( 9, "Errno = %d\n", errno )); -- WRONG!!
 *	    Debug( 9, "Errno = %d\n", errno );    -- WRONG!!
 */

#ifdef DEBUG
# define Debug( stuff )			_DEBUG stuff
# define DebugInit( stuff )		_DEBUGInit stuff
# define DebugEnd( stuff )		_DEBUGEnd stuff
# define DebugChgLevel( stuff )	_DEBUGChgLevel stuff
#else
# define Debug( stuff )			;
# define DebugInit( stuff )		;
# define DebugEnd( stuff )		;
# define DebugChgLevel( stuff )	;
#endif

/* Local struct & new type definition */

/* External functions */

/* External variables */

/* Global Variables */

/* Local Variables */

#ifdef __cplusplus
}
#endif

#endif
