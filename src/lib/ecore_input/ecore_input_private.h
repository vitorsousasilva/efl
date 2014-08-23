#ifndef _ECORE_INPUT_PRIVATE_H
# define _ECORE_INPUT_PRIVATE_H

# ifdef HAVE_CONFIG_H
#  include <config.h>
# endif

# ifdef HAVE_SYSTEMD_LOGIN
#  include <unistd.h>
#  include <systemd/sd-login.h>
# endif

# ifdef HAVE_LIBINPUT
#  include <libudev.h>
#  include <libinput.h>
#  include <dbus/dbus.h>
# endif

# include "Ecore.h"
# include "ecore_private.h"

# include "Ecore_Input.h"

extern int _ecore_input_log_dom;

# ifdef HAVE_SYSTEMD_LOGIN
extern char *_ecore_input_session_id;
# endif

# ifdef HAVE_LIBINPUT
extern struct udev *_ecore_input_udev;
extern Eina_List *_ecore_input_devices;
# endif

# ifdef ECORE_INPUT_DEFAULT_LOG_COLOR
#  undef ECORE_INPUT_DEFAULT_LOG_COLOR 
# endif

# define ECORE_INPUT_DEFAULT_LOG_COLOR EINA_COLOR_BLUE

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_ecore_input_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_ecore_input_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_ecore_input_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_ecore_input_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_ecore_input_log_dom, __VA_ARGS__)

struct _Ecore_Input_Device
{
   int fd;

   const char *seat;
   const char *name;
   const char *output;

   Ecore_Input_Device_Type type;

   Ecore_Fd_Handler *hdlr;

#ifdef HAVE_LIBINPUT
   struct udev_monitor *monitor;
#endif

   Eina_Bool enabled : 1;
   Eina_Bool suspended : 1;
};

Eina_Bool _ecore_input_dbus_init(void);
void _ecore_input_dbus_shutdown(void);
int _ecore_input_dbus_device_open(const char *device);
void _ecore_input_dbus_device_close(int fd);

#endif
