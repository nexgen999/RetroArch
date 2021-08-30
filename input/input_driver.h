/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __INPUT_DRIVER__H
#define __INPUT_DRIVER__H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <boolean.h>
#include <retro_common_api.h>
#include <retro_inline.h>
#include <libretro.h>
#include <retro_miscellaneous.h>

#include "input_defines.h"
#include "input_types.h"

#include "../msg_hash.h"
#include "include/hid_types.h"
#include "include/hid_driver.h"
#include "include/gamepad.h"
#include "../configuration.h"

RETRO_BEGIN_DECLS

struct retro_keybind
{
   /**
    * Human-readable label for the control.
    */
   char     *joykey_label;

   /**
    * Human-readable label for an analog axis.
    */
   char     *joyaxis_label;

   /**
    * Joypad axis. Negative and positive axes are both represented by this variable.
    */
   uint32_t joyaxis;

   /**
    * Default joy axis binding value for resetting bind to default.
    */
   uint32_t def_joyaxis;

   /**
    * Used by input_{push,pop}_analog_dpad().
    */
   uint32_t orig_joyaxis;

   enum msg_hash_enums enum_idx;

   enum retro_key key;

   uint16_t id;

   /**
    * What mouse button ID has been mapped to this control.
    */
   uint16_t mbutton;

   /**
    * Joypad key. Joypad POV (hats) are embedded into this key as well.
    **/
   uint16_t joykey;

   /**
    * Default key binding value (for resetting bind).
    */
   uint16_t def_joykey;

   /**
    * Determines whether or not the binding is valid.
    */
   bool valid;
};

extern struct retro_keybind input_config_binds[MAX_USERS][RARCH_BIND_LIST_END];
extern struct retro_keybind input_autoconf_binds[MAX_USERS][RARCH_BIND_LIST_END];

struct rarch_joypad_info
{
   const struct retro_keybind *auto_binds;
   float axis_threshold;
   uint16_t joy_idx;
};

typedef struct
{
   unsigned name_index;
   uint16_t vid;
   uint16_t pid;
   char joypad_driver[32];
   char name[256];
   char display_name[256];
   char config_path[PATH_MAX_LENGTH]; /* Path to the RetroArch config file */
   char config_name[PATH_MAX_LENGTH]; /* Base name of the RetroArch config file */
   bool autoconfigured;
} input_device_info_t;

typedef struct
{
   char display_name[256];
} input_mouse_info_t;

/**
 * Organizes the functions and data structures of each driver that are accessed
 * by other parts of the input code. The input_driver structs are the "interface"
 * between RetroArch and the input driver.
 * 
 * Every driver must establish an input_driver struct with pointers to its own 
 * implementations of these functions, and each of those input_driver structs is
 * declared below.
 */
struct input_driver
{
   /** 
    * Initializes input driver.
    * 
    * @param joypad_driver  Name of the joypad driver associated with the
    *                       input driver
    */
   void *(*init)(const char *joypad_driver);

  /**
    * Called once every frame to poll input. This function pointer can be set
    * to NULL if not supported by the input driver, for example if a joypad
    * driver is responsible for polling on a particular driver/platform.
    *
    * @param data  the input state struct
    */
   void (*poll)(void *data);

   /** 
    * Queries state for a specified control on a specified input port. This
    * function pointer can be set to NULL if not supported by the input driver,
    * for example if a joypad driver is responsible for quering state for a
    * particular driver/platform.
    *
    * @param joypad_data      Input state struct, defined by the input driver
    * @param sec_joypad_data  Input state struct for secondary input devices (eg
    *                         MFi controllers), defined by a secondary driver.
    *                         Queried state to be returned is the logical OR of
    *                         joypad_data and sec_joypad_data. May be NULL.
    * @param joypad_info      Info struct for the controller to be queried,
    *                         with hardware device ID and autoconfig mapping.
    * @param retro_keybinds   Structure for control mappings for all libretro
    *                         input device abstractions
    * @param keyboard_mapping_blocked 
    *                         If true, disregard custom keyboard mapping
    * @param port             Which RetroArch port is being polled
    * @param device           Which libretro abstraction is being polled 
    *                         (RETRO_DEVICE_ID_RETROPAD, RETRO_DEVICE_ID_MOUSE)
    * @param index            For controls with more than one axis or multiple
    *                         simultaneous inputs, such as an analog joystick
    *                         or touchpad.
    * @param id               Which control is being polled
    *                         (eg RETRO_DEVICE_ID_JOYPAD_START)
    *
    * @return 1 for pressed digital control, 0 for non-pressed digital control.
    *          Values in the range of a signed 16-bit integer,[-0x8000, 0x7fff]
    */
   int16_t (*input_state)(void *data,
         const input_device_driver_t *joypad_data,
         const input_device_driver_t *sec_joypad_data,
         rarch_joypad_info_t *joypad_info,
         const struct retro_keybind **retro_keybinds,
         bool keyboard_mapping_blocked,
         unsigned port, unsigned device, unsigned index, unsigned id);

   /**
    * Frees the input struct.
    * 
    * @param data The input state struct.
    */
   void (*free)(void *data);

   /**
    * Sets the state related for sensors, such as polling rate or to deactivate
    * the sensor entirely, etc. This function pointer may be set to NULL if
    * setting sensor values is not supported.
    * 
    * @param data    The input state struct
    * @param port
    * @param effect  Sensor action
    * @param rate    Sensor rate update
    * 
    * @return true if the operation is successful.
   **/
   bool (*set_sensor_state)(void *data, unsigned port,
         enum retro_sensor_action action, unsigned rate);

   /**
    * Retrieves the sensor state associated with the provided port and ID. This
    * function pointer may be set to NULL if retreiving sensor state is not
    * supported.
    * 
    * @param data  The input state struct
    * @param port
    * @param id    Sensor ID
    * 
    * @return The current state associated with the port and ID as a float
    **/
   float (*get_sensor_input)(void *data, unsigned port, unsigned id);

   /**
    * The means for an input driver to indicate to RetroArch which libretro
    * input abstractions the driver supports.
    * 
    * @param data  The input state struct.
    * 
    * @return A unit64_t composed via bitwise operators.
    */
   uint64_t (*get_capabilities)(void *data);

   /**
    * The human-readable name of the input driver.
    */
   const char *ident;

   /**
    * Grab or ungrab the mouse according to the value of `state`. This function
    * pointer can be set to NULL if the driver does not support grabbing the
    * mouse.
    * 
    * @param data   The input state struct
    * @param state  True to grab the mouse, false to ungrab
    */
   void (*grab_mouse)(void *data, bool state);

   /**
    * Check to see if the input driver has claimed stdin, and therefore it is
    * not available for other input. This function pointercan be set to NULL if
    * the driver does not support claiming stdin.
    * 
    * @param data  The input state struct
    * 
    * @return True if the input driver has claimed stdin.
    */
   bool (*grab_stdin)(void *data);
};

struct rarch_joypad_driver
{
   void *(*init)(void *data);
   bool (*query_pad)(unsigned);
   void (*destroy)(void);
   int32_t (*button)(unsigned, uint16_t);
   int16_t (*state)(rarch_joypad_info_t *joypad_info,
         const struct retro_keybind *binds, unsigned port);
   void (*get_buttons)(unsigned, input_bits_t *);
   int16_t (*axis)(unsigned, uint32_t);
   void (*poll)(void);
   bool (*set_rumble)(unsigned, enum retro_rumble_effect, uint16_t);
   const char *(*name)(unsigned);

   const char *ident;
};

typedef struct
{
   /* pointers */
   input_driver_t                *current_driver;
   void                          *current_data;
   const input_device_driver_t   *primary_joypad;        /* ptr alignment */
   const input_device_driver_t   *secondary_joypad;      /* ptr alignment */

   /* primitives */
   bool        nonblocking_flag;
} input_driver_state_t;


void input_driver_init_joypads(void);

/**
 * Get an enumerated list of all input driver names
 *
 * @return string listing of all input driver names, separated by '|'.
 **/
const char* config_get_input_driver_options(void);

/**
 * Sets the rumble state.
 * 
 * @param driver_state
 * @param port          User number.
 * @param joy_idx
 * @param effect        Rumble effect.
 * @param strength      Strength of rumble effect.
 *
 * @return true if the rumble state has been successfully set
 **/
bool input_driver_set_rumble(
         input_driver_state_t *driver_state, unsigned port, unsigned joy_idx, 
         enum retro_rumble_effect effect, uint16_t strength);

/**
 * Sets the sensor state.
 * 
 * @param driver_state
 * @param port
 * @param sensors_enable
 * @param effect        Sensor action
 * @param rate          Sensor rate update
 *
 * @return true if the sensor state has been successfully set
 **/
bool input_driver_set_sensor(
         input_driver_state_t *driver_state, unsigned port, bool sensors_enable,
         enum retro_sensor_action action, unsigned rate);

/**
 * Retrieves the sensor state associated with the provided port and ID. 
 * 
 * @param driver_state
 * @param port
 * @param sensors_enable
 * @param id            Sensor ID
 *
 * @return The current state associated with the port and ID as a float
 **/
float input_driver_get_sensor(
         input_driver_state_t *driver_state,
         unsigned port, bool sensors_enable, unsigned id);

/**
 * Get an enumerated list of all joypad driver names
 *
 * @return String listing of all joypad driver names, separated by '|'.
 **/
const char* config_get_joypad_driver_options(void);

/**
 * Initialize a joypad driver of name ident. If ident points to NULL or a 
 * zero-length string, equivalent to calling input_joypad_init_first().
 *
 * @param ident  identifier of driver to initialize.
 * @param data   joypad state data pointer, which can be NULL and will be
 *               initialized by the new joypad driver, if one is found.
 *
 * @return The joypad driver if found, otherwise NULL.
 **/
const input_device_driver_t *input_joypad_init_driver(
      const char *ident, void *data);

/**
 * Takes as input analog key identifiers and converts them to corresponding
 * bind IDs ident_minus and ident_plus.
 * 
 * @param idx          Analog key index (eg RETRO_DEVICE_INDEX_ANALOG_LEFT)
 * @param ident        Analog key identifier (eg RETRO_DEVICE_ID_ANALOG_X)
 * @param ident_minus  Bind ID minus, will be set by function.
 * @param ident_plus   Bind ID plus,  will be set by function.
 */
#define input_conv_analog_id_to_bind_id(idx, ident, ident_minus, ident_plus) \
   switch ((idx << 1) | ident) \
   { \
      case (RETRO_DEVICE_INDEX_ANALOG_LEFT << 1) | RETRO_DEVICE_ID_ANALOG_X: \
         ident_minus = RARCH_ANALOG_LEFT_X_MINUS; \
         ident_plus  = RARCH_ANALOG_LEFT_X_PLUS; \
         break; \
      case (RETRO_DEVICE_INDEX_ANALOG_LEFT << 1) | RETRO_DEVICE_ID_ANALOG_Y: \
         ident_minus = RARCH_ANALOG_LEFT_Y_MINUS; \
         ident_plus  = RARCH_ANALOG_LEFT_Y_PLUS; \
         break; \
      case (RETRO_DEVICE_INDEX_ANALOG_RIGHT << 1) | RETRO_DEVICE_ID_ANALOG_X: \
         ident_minus = RARCH_ANALOG_RIGHT_X_MINUS; \
         ident_plus  = RARCH_ANALOG_RIGHT_X_PLUS; \
         break; \
      case (RETRO_DEVICE_INDEX_ANALOG_RIGHT << 1) | RETRO_DEVICE_ID_ANALOG_Y: \
         ident_minus = RARCH_ANALOG_RIGHT_Y_MINUS; \
         ident_plus  = RARCH_ANALOG_RIGHT_Y_PLUS; \
         break; \
   }

/**
 * Registers a newly connected pad with RetroArch.
 * 
 * @param port    Joystick number
 * @param driver  Handle for joypad driver handling joystick's input
 **/
void input_pad_connect(unsigned port, input_device_driver_t *driver);


/**
 * line_complete callback (when carriage return is pressed)
 *
 * @param userdata User data which will be passed to subsequent callbacks.
 * @param line      the line of input, which can be NULL.
 **/
typedef void (*input_keyboard_line_complete_t)(void *userdata,
      const char *line);

/**
 * Callback for keypress events
 * 
 * @param userdata The user data that was passed through from the keyboard press callback.
 * @param code      keycode
 **/
typedef bool (*input_keyboard_press_t)(void *userdata, unsigned code);

struct input_keyboard_ctx_wait
{
   void *userdata;
   input_keyboard_press_t cb;
};

/**
 * Called by drivers when keyboard events are fired. Interfaces with the global
 * driver struct and libretro callbacks.
 * 
 * @param down       Was Keycode pressed down?
 * @param code       Keycode.
 * @param character  Character inputted.
 * @param mod        TODO/FIXME/???
 **/
void input_keyboard_event(bool down, unsigned code, uint32_t character,
      uint16_t mod, unsigned device);


/*************************************/
#ifdef HAVE_HID
#include "include/hid_driver.h"

/**
 * Get an enumerated list of all HID driver names
 * 
 * @return String listing of all HID driver names, separated by '|'.
 **/
const char* config_get_hid_driver_options(void);

/**
 * Finds first suitable HID driver and initializes.
 *
 * @return HID driver if found, otherwise NULL.
 **/
const hid_driver_t *input_hid_init_first(void);

/**
 * Get a pointer to the HID driver data structure
 * 
 * @return Pointer to hid_data struct
 **/
const void *hid_driver_get_data(void);

/**
 * This should be called after we've invoked free() on the HID driver; the
 * memory will have already been freed so we need to reset the pointer.
 */
void hid_driver_reset_data(void);

#endif /* HAVE_HID */
/*************************************/


/**
 * Set the name of the device in the specified port
 * 
 * @param port
 */
void input_config_set_device_name(unsigned port, const char *name);

/**
 * Set the formatted "display name" of the device in the specified port
 * 
 * @param port
 */
void input_config_set_device_display_name(unsigned port, const char *name);
void input_config_set_mouse_display_name(unsigned port, const char *name);

/**
 * Set the configuration path for the device in the specified port
 * 
 * @param port
 * @param path The path of the device config.
 */
void input_config_set_device_config_path(unsigned port, const char *path);

/**
 * Set the configuration name for the device in the specified port
 * 
 * @param port
 * @param name The name of the config to set.
 */
void input_config_set_device_config_name(unsigned port, const char *name);

/**
 * Set the joypad driver for the device in the specified port
 * 
 * @param port
 * @param driver The driver to set the given port to.
 */
void input_config_set_device_joypad_driver(unsigned port, const char *driver);

/**
 * Set the vendor ID (vid) for the device in the specified port
 * 
 * @param port
 * @param vid The VID to set the given device port to.
 */
void input_config_set_device_vid(unsigned port, uint16_t vid);

/**
 * Set the pad ID (pid) for the device in the specified port
 * 
 * @param port
 * @param pid The PID to set the given device port to.
 */
void input_config_set_device_pid(unsigned port, uint16_t pid);

/**
 * Sets the autoconfigured flag for the device in the specified port
 *
 * @param port
 * @param autoconfigured Whether or nor the device is configured automatically.
 */
void input_config_set_device_autoconfigured(unsigned port, bool autoconfigured);

/**
 * Sets the name index number for the device in the specified port
 *
 * @param port
 * @param name_index The name index to set the device to use.
 */
void input_config_set_device_name_index(unsigned port, unsigned name_index);

/**
 * Sets the device type of the specified port
 * 
 * @param port
 * @param id The device type (RETRO_DEVICE_JOYPAD, RETRO_DEVICE_MOUSE, etc)
 */
void input_config_set_device(unsigned port, unsigned id);

/**
 * Registers a pad_connection_listener_interface with a function pointer that
 * is called when a joypad is connected. Only used by the wiiu_joypad driver.
 *
 * @param listener  a struct that implements pad_connection_listener_interface
 */
void set_connection_listener(pad_connection_listener_t *listener);

/* Clear input_device_info */
void input_config_clear_device_name(unsigned port);
void input_config_clear_device_display_name(unsigned port);
void input_config_clear_device_config_path(unsigned port);
void input_config_clear_device_config_name(unsigned port);
void input_config_clear_device_joypad_driver(unsigned port);

unsigned input_config_get_device_count(void);

unsigned *input_config_get_device_ptr(unsigned port);

unsigned input_config_get_device(unsigned port);

/* Get input_device_info */
const char *input_config_get_device_name(unsigned port);
const char *input_config_get_device_display_name(unsigned port);
const char *input_config_get_mouse_display_name(unsigned port);
const char *input_config_get_device_config_path(unsigned port);
const char *input_config_get_device_config_name(unsigned port);
const char *input_config_get_device_joypad_driver(unsigned port);

/**
 * Retrieves the vendor id (vid) of a connected controller
 * 
 * @param port
 *
 * @return the vendor id VID of the device
 */
uint16_t input_config_get_device_vid(unsigned port);

/**
 * Retrieves the pad id (pad) of a connected controller
 * 
 * @param port
 *
 * @return the port id PID of the device
 */
uint16_t input_config_get_device_pid(unsigned port);

/**
 * Returns the value of the autoconfigured flag for the specified device
 *
 * @param port
 *
 * @return the autoconfigured flag
 */
bool input_config_get_device_autoconfigured(unsigned port);

/**
 * Get the name index number for the device in this port
 * 
 * @param port
 *
 * @return the name index for this device
 */
unsigned input_config_get_device_name_index(unsigned port);


/*****************************************************************************/

/**
 * Retrieve the device name char pointer.
 *
 * @deprecated input_config_get_device_name_ptr is required by linuxraw_joypad
 * and parport_joypad. These drivers should be refactored such that this
 * low-level access is not required.
 * 
 * @param port
 *
 * @return a pointer to the device name on the specified port
 */
char *input_config_get_device_name_ptr(unsigned port);

/**
 * Get the size of the device name.
 *
 * @deprecated input_config_get_device_name_size is required by linuxraw_joypad
 * and parport_joypad. These drivers should be refactored such that this
 * low-level access is not required.
 * 
 * @param port
 *
 * @return the size of the device name on the specified port
 */
size_t input_config_get_device_name_size(unsigned port);

/*****************************************************************************/

const struct retro_keybind *input_config_get_bind_auto(unsigned port, unsigned id);

void input_config_reset_autoconfig_binds(unsigned port);
void input_config_reset(void);

#if defined(ANDROID)
#define DEFAULT_MAX_PADS 8
#define ANDROID_KEYBOARD_PORT DEFAULT_MAX_PADS
#elif defined(_3DS)
#define DEFAULT_MAX_PADS 1
#elif defined(SWITCH) || defined(HAVE_LIBNX)
#define DEFAULT_MAX_PADS 8
#elif defined(WIIU)
#ifdef WIIU_HID
#define DEFAULT_MAX_PADS 16
#else
#define DEFAULT_MAX_PADS 5
#endif /* WIIU_HID */
#elif defined(DJGPP)
#define DEFAULT_MAX_PADS 1
#define DOS_KEYBOARD_PORT DEFAULT_MAX_PADS
#elif defined(XENON)
#define DEFAULT_MAX_PADS 4
#elif defined(VITA) || defined(SN_TARGET_PSP2)
#define DEFAULT_MAX_PADS 4
#elif defined(PSP)
#define DEFAULT_MAX_PADS 1
#elif defined(PS2)
#define DEFAULT_MAX_PADS 8
#elif defined(GEKKO) || defined(HW_RVL)
#define DEFAULT_MAX_PADS 4
#elif defined(HAVE_ODROIDGO2)
#define DEFAULT_MAX_PADS 1
#elif defined(__linux__) || (defined(BSD) && !defined(__MACH__))
#define DEFAULT_MAX_PADS 8
#elif defined(__QNX__)
#define DEFAULT_MAX_PADS 8
#elif defined(__PS3__)
#define DEFAULT_MAX_PADS 7
#elif defined(_XBOX)
#define DEFAULT_MAX_PADS 4
#elif defined(HAVE_XINPUT) && !defined(HAVE_DINPUT)
#define DEFAULT_MAX_PADS 4
#elif defined(DINGUX)
#define DEFAULT_MAX_PADS 2
#else
#define DEFAULT_MAX_PADS 16
#endif /* defined(ANDROID) */

extern input_device_driver_t *joypad_drivers[];
extern input_driver_t *input_drivers[];
#ifdef HAVE_HID
extern hid_driver_t *hid_drivers[];
#endif

extern input_driver_t input_android;
extern input_driver_t input_sdl;
extern input_driver_t input_sdl_dingux;
extern input_driver_t input_dinput;
extern input_driver_t input_x;
extern input_driver_t input_ps4;
extern input_driver_t input_ps3;
extern input_driver_t input_psp;
extern input_driver_t input_ps2;
extern input_driver_t input_ctr;
extern input_driver_t input_switch;
extern input_driver_t input_xenon360;
extern input_driver_t input_gx;
extern input_driver_t input_wiiu;
extern input_driver_t input_xinput;
extern input_driver_t input_uwp;
extern input_driver_t input_linuxraw;
extern input_driver_t input_udev;
extern input_driver_t input_cocoa;
extern input_driver_t input_qnx;
extern input_driver_t input_rwebinput;
extern input_driver_t input_dos;
extern input_driver_t input_winraw;
extern input_driver_t input_wayland;

extern input_device_driver_t dinput_joypad;
extern input_device_driver_t linuxraw_joypad;
extern input_device_driver_t parport_joypad;
extern input_device_driver_t udev_joypad;
extern input_device_driver_t xinput_joypad;
extern input_device_driver_t sdl_joypad;
extern input_device_driver_t sdl_dingux_joypad;
extern input_device_driver_t ps4_joypad;
extern input_device_driver_t ps3_joypad;
extern input_device_driver_t psp_joypad;
extern input_device_driver_t ps2_joypad;
extern input_device_driver_t ctr_joypad;
extern input_device_driver_t switch_joypad;
extern input_device_driver_t xdk_joypad;
extern input_device_driver_t gx_joypad;
extern input_device_driver_t wiiu_joypad;
extern input_device_driver_t hid_joypad;
extern input_device_driver_t android_joypad;
extern input_device_driver_t qnx_joypad;
extern input_device_driver_t mfi_joypad;
extern input_device_driver_t dos_joypad;
extern input_device_driver_t rwebpad_joypad;

#ifdef HAVE_HID
extern hid_driver_t iohidmanager_hid;
extern hid_driver_t btstack_hid;
extern hid_driver_t libusb_hid;
extern hid_driver_t wiiusb_hid;
#endif /* HAVE_HID */

RETRO_END_DECLS

#endif /* __INPUT_DRIVER__H */
