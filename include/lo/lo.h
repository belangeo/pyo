/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

#ifndef LO_H
#define LO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file lo.h The liblo main headerfile and high-level API functions.
 */

#include "lo/lo_endian.h"
#include "lo/lo_types.h"
#include "lo/lo_osc_types.h"
#include "lo/lo_errors.h"
#include "lo/lo_lowlevel.h"

/**
 * \defgroup liblo High-level OSC API
 *
 * Defines the high-level API functions necessary to implement OSC support.
 * Should be adequate for most applications, but if you require lower level
 * control you can use the functions defined in lo_lowlevel.h
 * @{
 */

/**
 * \brief Declare an OSC destination, given IP address and port number.
 * Same as lo_address_new_with_proto(), but using UDP.
 *
 * \param host An IP address or number, or NULL for the local machine.
 * \param port a decimal port number or service name.
 *
 * The lo_address object may be used as the target of OSC messages.
 *
 * Note: if you wish to receive replies from the target of this address, you
 * must first create a lo_server_thread or lo_server object which will receive
 * the replies. The last lo_server(_thread) object creted will be the receiver.
 */
lo_address lo_address_new(const char *host, const char *port);

/**
 * \brief Declare an OSC destination, given IP address and port number,
 * specifying protocol.
 *
 * \param proto The protocol to use, must be one of LO_UDP, LO_TCP or LO_UNIX.
 * \param host An IP address or number, or NULL for the local machine.
 * \param port a decimal port number or service name.
 *
 * The lo_address object may be used as the target of OSC messages.
 *
 * Note: if you wish to receive replies from the target of this address, you
 * must first create a lo_server_thread or lo_server object which will receive
 * the replies. The last lo_server(_thread) object creted will be the receiver.
 */
lo_address lo_address_new_with_proto(int proto, const char *host, const char *port);

/**
 * \brief Create a lo_address object from an OSC URL.
 *
 * example: \c "osc.udp://localhost:4444/my/path/"
 */
lo_address lo_address_new_from_url(const char *url);

/**
 * \brief Free the memory used by the lo_address object
 */ 
void lo_address_free(lo_address t);

/**
 * \brief Set the Time-to-Live value for a given target address.
 * 
 * This is required for sending multicast UDP messages.  A value of 1
 * (the usual case) keeps the message within the subnet, while 255
 * means a global, unrestricted scope.
 * 
 * \param t An OSC address.
 * \param ttl An integer specifying the scope of a multicast UDP message.
 */ 
void lo_address_set_ttl(lo_address t, int ttl);

/**
 * \brief Get the Time-to-Live value for a given target address.
 * 
 * \param t An OSC address.
 * \return An integer specifying the scope of a multicast UDP message.
 */ 
int lo_address_get_ttl(lo_address t);

/**
 * \brief Send a OSC formatted message to the address specified.
 *
 * \param targ The target OSC address
 * \param path The OSC path the message will be delivered to
 * \param type The types of the data items in the message, types are defined in
 * lo_osc_types.h
 * \param ... The data values to be transmitted. The types of the arguments
 * passed here must agree with the types specified in the type parameter.
 *
 * example:
 * \code
 * lo_send(t, "/foo/bar", "ff", 0.1f, 23.0f);
 * \endcode
 *
 * \return -1 on failure.
 */
int lo_send(lo_address targ, const char *path, const char *type, ...);

/**
 * \brief Send a OSC formatted message to the address specified, 
 * from the same socket as the specificied server.
 *
 * \param targ The target OSC address
 * \param from The server to send message from   (can be NULL to use new socket)
 * \param ts   The OSC timetag timestamp at which the message will be processed 
 * (can be LO_TT_IMMEDIATE if you don't want to attach a timetag)
 * \param path The OSC path the message will be delivered to
 * \param type The types of the data items in the message, types are defined in
 * lo_osc_types.h
 * \param ... The data values to be transmitted. The types of the arguments
 * passed here must agree with the types specified in the type parameter.
 *
 * example:
 * \code
 * serv = lo_server_new(NULL, err);
 * lo_server_add_method(serv, "/reply", "ss", reply_handler, NULL);
 * lo_send_from(t, serv, LO_TT_IMMEDIATE, "/foo/bar", "ff", 0.1f, 23.0f);
 * \endcode
 *
 * \return on success, the number of bytes sent, or -1 on failure.
 */
int lo_send_from(lo_address targ, lo_server from, lo_timetag ts, 
	       		const char *path, const char *type, ...);

/**
 * \brief Send a OSC formatted message to the address specified, scheduled to
 * be dispatch at some time in the future.
 *
 * \param targ The target OSC address
 * \param ts The OSC timetag timestamp at which the message will be processed
 * \param path The OSC path the message will be delivered to
 * \param type The types of the data items in the message, types are defined in
 * lo_osc_types.h
 * \param ... The data values to be transmitted. The types of the arguments
 * passed here must agree with the types specified in the type parameter.
 *
 * example:
 * \code
 * lo_timetag now;<br>
 * lo_timetag_now(&now);<br>
 * lo_send_timestamped(t, now, "/foo/bar", "ff", 0.1f, 23.0f);
 * \endcode
 *
 * \return on success, the number of bytes sent, or -1 on failure.
 */
int lo_send_timestamped(lo_address targ, lo_timetag ts, const char *path,
	       		const char *type, ...);

/**
 * \brief Return the error number from the last failed lo_send() or
 * lo_address_new() call
 */
int lo_address_errno(lo_address a);

/**
 * \brief Return the error string from the last failed lo_send() or
 * lo_address_new() call
 */
const char *lo_address_errstr(lo_address a);

/**
 * \brief Create a new server thread to handle incoming OSC
 * messages.
 *
 * Server threads take care of the message reception and dispatch by
 * transparently creating a system thread to handle incoming messages.
 * Use this if you do not want to handle the threading yourself.
 *
 * \param port If NULL is passed then an unused port will be chosen by the
 * system, its number may be retrieved with lo_server_thread_get_port()
 * so it can be passed to clients. Otherwise a decimal port number, service
 * name or UNIX domain socket path may be passed.
 * \param err_h A function that will be called in the event of an error being
 * raised. The function prototype is defined in lo_types.h
 */
lo_server_thread lo_server_thread_new(const char *port, lo_err_handler err_h);

/**
 * \brief Create a new server thread to handle incoming OSC
 * messages, and join a UDP multicast group.
 *
 * Server threads take care of the message reception and dispatch by
 * transparently creating a system thread to handle incoming messages.
 * Use this if you do not want to handle the threading yourself.
 *
 * \param group The multicast group to join.  See documentation on IP
 * multicast for the acceptable address range; e.g., http://tldp.org/HOWTO/Multicast-HOWTO-2.html
 * \param port If NULL is passed then an unused port will be chosen by the
 * system, its number may be retrieved with lo_server_thread_get_port()
 * so it can be passed to clients. Otherwise a decimal port number, service
 * name or UNIX domain socket path may be passed.
 * \param err_h A function that will be called in the event of an error being
 * raised. The function prototype is defined in lo_types.h
 */
lo_server_thread lo_server_thread_new_multicast(const char *group, const char *port,
                                                lo_err_handler err_h);

/**
 * \brief Create a new server thread to handle incoming OSC
 * messages, specifying protocol.
 *
 * Server threads take care of the message reception and dispatch by
 * transparently creating a system thread to handle incoming messages.
 * Use this if you do not want to handle the threading yourself.
 *
 * \param port If NULL is passed then an unused port will be chosen by the
 * system, its number may be retrieved with lo_server_thread_get_port()
 * so it can be passed to clients. Otherwise a decimal port number, service
 * name or UNIX domain socket path may be passed.
 * \param proto The protocol to use, should be one of LO_UDP, LO_TCP or LO_UNIX.
 * \param err_h A function that will be called in the event of an error being
 * raised. The function prototype is defined in lo_types.h
 */
lo_server_thread lo_server_thread_new_with_proto(const char *port, int proto,
				   lo_err_handler err_h);

/**
 * \brief Free memory taken by a server thread
 *
 * Frees the memory, and, if currently running will stop the associated thread.
 */
void lo_server_thread_free(lo_server_thread st);

/**
 * \brief Add an OSC method to the specifed server thread.
 *
 * \param st The server thread the method is to be added to.
 * \param path The OSC path to register the method to. If NULL is passed the
 * method will match all paths.
 * \param typespec The typespec the method accepts. Incoming messages with
 * similar typespecs (e.g. ones with numerical types in the same position) will
 * be coerced to the typespec given here.
 * \param h The method handler callback function that will be called it a
 * matching message is received
 * \param user_data A value that will be passed to the callback function, h,
 * when its invoked matching from this method.
 */
lo_method lo_server_thread_add_method(lo_server_thread st, const char *path,
                               const char *typespec, lo_method_handler h,
                               void *user_data);
/**
 * \brief Delete an OSC method from the specifed server thread.
 *
 * \param st The server thread the method is to be removed from.
 * \param path The OSC path of the method to delete. If NULL is passed the
 * method will match the generic handler.
 * \param typespec The typespec the method accepts.
 */
void lo_server_thread_del_method(lo_server_thread st, const char *path,
				 const char *typespec);

/**
 * \brief Start the server thread
 *
 * \param st the server thread to start.
 * \return Less than 0 on failure, 0 on success.
 */
int lo_server_thread_start(lo_server_thread st);

/**
 * \brief Stop the server thread
 *
 * \param st the server thread to start.
 * \return Less than 0 on failure, 0 on success.
 */
int lo_server_thread_stop(lo_server_thread st);

/**
 * \brief Return the port number that the server thread has bound to.
 */
int lo_server_thread_get_port(lo_server_thread st);

/**
 * \brief Return a URL describing the address of the server thread.
 *
 * Return value must be free()'d to reclaim memory.
 */
char *lo_server_thread_get_url(lo_server_thread st);

/**
 * \brief Return the lo_server for a lo_server_thread
 *
 * This function is useful for passing a thread's lo_server 
 * to lo_send_from().
 */
lo_server lo_server_thread_get_server(lo_server_thread st);

/** \brief Return true if there are scheduled events (eg. from bundles) waiting
 * to be dispatched by the thread */
int lo_server_thread_events_pending(lo_server_thread st);

/**
 * \brief Create a new OSC blob type.
 *
 * \param size The amount of space to allocate in the blob structure.
 * \param data The data that will be used to initialise the blob, should be
 * size bytes long.
 */
lo_blob lo_blob_new(int32_t size, const void *data);

/**
 * \brief Free the memory taken by a blob
 */
void lo_blob_free(lo_blob b);

/**
 * \brief Return the amount of valid data in a lo_blob object.
 *
 * If you want to know the storage size, use lo_arg_size().
 */
uint32_t lo_blob_datasize(lo_blob b);

/**
 * \brief Return a pointer to the start of the blob data to allow contents to
 * be changed.
 */
void *lo_blob_dataptr(lo_blob b);

/** @} */

#include "lo/lo_macros.h"

#ifdef __cplusplus
}
#endif

#endif
