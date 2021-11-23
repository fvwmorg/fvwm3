New Parser
==========

How the "new" parser "cmdparser_old" works at the moment
--------------------------------------------------------

DV: The point in implementing "cmdparser_old" was to make command
line parsers pluggable - a requirement for a smooth transition
away from old style parsing to a new, better syntax and parser.

Although many files hat to be touched in the new implementation,
the important things are in

 - cmdparser_old.[ch]
     Implementation of the backwards compatioble "new old" parser.
     The header file is only included from functions.c to the the
     hooks structure.  Everything else is private.

 - cmdparser_hooks.h
     Definitions of the hook functions that are used from
     functions.c.  No other file must include it.

 - cmdparser.h
     The parsing interface that needs to be provided by all
     concurring parser implementation.  The cmdparser_context_t is
     the interface to all functions that need to parse something.
     The other stypes are only interesting in functions.c

 - functable_complex.[ch]
     Code dealing with defining, destroying and finding complex
     functions.  This strewn over several other files before.

 - functions.c
     Huge changes to interface with the parser by using the parser
     hooks.

cmdparser interface
~~~~~~~~~~~~~~~~~~~

cmdparser_hooks_t

  A structure with function pointers that provide the parsing
  functionality.  The details are documented in the header file.
  Parsing steps are in order:

  * Make a context with create_context().
  * Call handle_line_start to deal with leading whitespace and
    comments etc.
  * handle_line_prefix() strips command prefixes and returns them
    as flags.
  * parse_command_name() figures out the token in command position.
  * is_module_config() tells whether the command line is a module
    config.  Currently only used internally in the parser.
  * expand_command_name() replaces variables in the line with
    their values.  This is not done for command with the
    FUNC_DONT_EXPAND_COMMAND flag in functable.c.
  * The expanded command line is in allocated memory in the expline
    variable.  If the called wants to store that, it can call
    release_expanded_line() and then copy the pointer to another
    place.  The caller then has to free the expline once it's no
    longer needed.
  * find_something_to_execute() figures out the command or function
    to execute if it's a module config line.
  * When a function or command is called, it is passed the
    cmdparser_context_t structure.  With cmdparser_old, the first
    ten positional arguments are pre-tokenised.
  * Finally, the bits allocated inside the context are free'd with
    destroy_context().

cmdparser_context_t

  A structure with all the data and internal state of the parsed
  command line.  As this is currently only used by functions.c, it
  is not yet well defined which bits may be accessed by commands
  that need parsing.  -> future work.

  At the moment, "all_pos_args_string" and "pos_arg_tokens" are a
  bit unclear to me.

Future work
~~~~~~~~~~~

  An immediate next step to make better use of the new parser
  interface is to get rid of PeekToken(), GetNextToken() etc. in the
  Cmd_...() functions.  Provide parsing functions that are aware of
  the parser being used and the context structure.
