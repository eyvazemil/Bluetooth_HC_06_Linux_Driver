/**
 * @brief File declares macros that will be used by different projects.
 */

#ifndef CUSTOM_MACROS_H
#define CUSTOM_MACROS_H

/**
 * Create a macro for printing the messages only if our module runs in debug 
 * mode. In non-debug mode, the macro for printing does nothing.
 */
#ifdef DEBUG_MODE
#	define PRINT_DEBUG(fmt, args...) printk(KERN_ALERT fmt, ## args)
#else
#	define PRINT_DEBUG(fmt, args...)
#endif

#endif // CUSTOM_MACROS_H
