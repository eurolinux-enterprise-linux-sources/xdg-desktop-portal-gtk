#define _GNU_SOURCE 1

#include "config.h"

#include <gtk/gtk.h>

#include <gio/gio.h>

#include "xdg-desktop-portal-dbus.h"
#include "shell-dbus.h"
#include "utils.h"

#include "inhibit.h"
#include "request.h"

enum {
  INHIBIT_LOGOUT  = 1,
  INHIBIT_SWITCH  = 2,
  INHIBIT_SUSPEND = 4,
  INHIBIT_IDLE    = 8
};

static OrgGnomeSessionManager *session;

static void
uninhibit_done_gnome (GObject *source,
                      GAsyncResult *result,
                      gpointer data)
{
  g_autoptr(GError) error = NULL;

  if (!org_gnome_session_manager_call_uninhibit_finish (session,
                                                        result,
                                                        &error))
    g_warning ("Backend call failed: %s", error->message);
}

static gboolean
handle_close_gnome (XdpImplRequest *object,
                    GDBusMethodInvocation *invocation,
                    gpointer data)
{
  Request *request = (Request *)object;
  guint cookie;

  /* If we get a Close call before the Inhibit call returned,
   * delay the uninhibit call until we have the cookie.
   */
  cookie = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (request), "cookie"));
  if (cookie)
    org_gnome_session_manager_call_uninhibit (session,
                                              cookie,
                                              NULL,
                                              uninhibit_done_gnome,
                                              NULL);
  else
    g_object_set_data (G_OBJECT (request), "closed", GINT_TO_POINTER (1));

  if (request->exported)
    request_unexport (request);

  xdp_impl_request_complete_close (object, invocation);

  return TRUE;
}

static void
inhibit_done_gnome (GObject *source,
                    GAsyncResult *result,
                    gpointer data)
{
  g_autoptr(Request) request = data;
  guint cookie = 0;
  gboolean closed;
  g_autoptr(GError) error = NULL;

  if (!org_gnome_session_manager_call_inhibit_finish (session,
                                                      &cookie,
                                                      result,
                                                      &error))
    g_warning ("Backend call failed: %s", error->message);

  closed = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (request), "closed"));

  if (closed)
    org_gnome_session_manager_call_uninhibit (session,
                                              cookie,
                                              NULL,
                                              uninhibit_done_gnome,
                                              NULL);
  else
    g_object_set_data (G_OBJECT (request), "cookie", GUINT_TO_POINTER (cookie));
}

static gboolean
handle_inhibit_gnome (XdpImplInhibit *object,
                      GDBusMethodInvocation *invocation,
                      const gchar *arg_handle,
                      const gchar *arg_app_id,
                      const gchar *arg_window,
                      guint arg_flags,
                      GVariant *arg_options)
{
  g_autoptr (Request) request = NULL;
  const char *sender;
  g_autoptr(GError) error = NULL;
  const char *reason;

  sender = g_dbus_method_invocation_get_sender (invocation);

  request = request_new (sender, arg_app_id, arg_handle);

  g_signal_connect (request, "handle-close", G_CALLBACK (handle_close_gnome), NULL);

  request_export (request, g_dbus_method_invocation_get_connection (invocation));

  if (!g_variant_lookup (arg_options, "reason", "&s", &reason))
    reason = "";

  org_gnome_session_manager_call_inhibit (session,
                                          arg_app_id,
                                          0, /* window */
                                          reason,
                                          arg_flags,
                                          NULL,
                                          inhibit_done_gnome,
                                          g_object_ref (request));

  xdp_impl_inhibit_complete_inhibit (object, invocation);

  return TRUE;
}

static OrgFreedesktopScreenSaver *screen_saver;

static void
uninhibit_done_fdo (GObject *source,
                    GAsyncResult *result,
                    gpointer data)
{
  g_autoptr(GError) error = NULL;

  if (!org_freedesktop_screen_saver_call_un_inhibit_finish (screen_saver,
                                                            result,
                                                            &error))
    g_warning ("Backend call failed: %s", error->message);
}

static gboolean
handle_close_fdo (XdpImplRequest *object,
                  GDBusMethodInvocation *invocation,
                  gpointer data)
{
  Request *request = (Request *)object;
  guint cookie;

  /* If we get a Close call before the Inhibit call returned,
   * delay the uninhibit call until we have the cookie.
   */
  cookie = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (request), "cookie"));
  if (cookie)
    org_freedesktop_screen_saver_call_un_inhibit (screen_saver,
                                                  cookie,
                                                  NULL,
                                                  uninhibit_done_fdo,
                                                  NULL);
  else
    g_object_set_data (G_OBJECT (request), "closed", GINT_TO_POINTER (1));

  if (request->exported)
    request_unexport (request);

  xdp_impl_request_complete_close (object, invocation);

  return TRUE;
}

static void
inhibit_done_fdo (GObject *source,
                  GAsyncResult *result,
                  gpointer data)
{
  g_autoptr(Request) request = data;
  guint cookie = 0;
  gboolean closed;
  g_autoptr(GError) error = NULL;

  if (!org_freedesktop_screen_saver_call_inhibit_finish (screen_saver,
                                                         &cookie,
                                                         result,
                                                         &error))
    g_warning ("Backend call failed: %s", error->message);

  closed = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (request), "closed"));

  if (closed)
    org_freedesktop_screen_saver_call_un_inhibit (screen_saver,
                                                  cookie,
                                                  NULL,
                                                  uninhibit_done_fdo,
                                                  NULL);
  else
    g_object_set_data (G_OBJECT (request), "cookie", GUINT_TO_POINTER (cookie));
}

static gboolean
handle_inhibit_fdo (XdpImplInhibit *object,
                    GDBusMethodInvocation *invocation,
                    const gchar *arg_handle,
                    const gchar *arg_app_id,
                    const gchar *arg_window,
                    guint arg_flags,
                    GVariant *arg_options)
{
  g_autoptr (Request) request = NULL;
  const char *sender;
  g_autoptr(GError) error = NULL;
  const char *reason;

  if ((arg_flags & ~INHIBIT_IDLE) != 0)
    {
      g_dbus_method_invocation_return_error (invocation,
                                             XDG_DESKTOP_PORTAL_ERROR,
                                             XDG_DESKTOP_PORTAL_ERROR_FAILED,
                                             "Inhibiting other than idle not supported");
      return TRUE;
    }

  sender = g_dbus_method_invocation_get_sender (invocation);

  request = request_new (sender, arg_app_id, arg_handle);

  g_signal_connect (request, "handle-close", G_CALLBACK (handle_close_fdo), NULL);

  request_export (request, g_dbus_method_invocation_get_connection (invocation));

  if (!g_variant_lookup (arg_options, "reason", "&s", &reason))
    reason = "";

  org_freedesktop_screen_saver_call_inhibit (screen_saver,
                                             arg_app_id,
                                             reason,
                                             NULL,
                                             inhibit_done_fdo,
                                             g_object_ref (request));

  xdp_impl_inhibit_complete_inhibit (object, invocation);

  return TRUE;
}

gboolean
inhibit_init (GDBusConnection *bus,
              GError **error)
{
  GDBusInterfaceSkeleton *helper;
  char *owner;

  helper = G_DBUS_INTERFACE_SKELETON (xdp_impl_inhibit_skeleton_new ());

  session = org_gnome_session_manager_proxy_new_sync (bus,
                                                      G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                      "org.gnome.SessionManager",
                                                      "/org/gnome/SessionManager",
                                                      NULL,
                                                      NULL);

  owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (session));
  if (owner)
    {
      g_free (owner);
      g_signal_connect (helper, "handle-inhibit", G_CALLBACK (handle_inhibit_gnome), NULL);
      g_debug ("Using org.gnome.SessionManager for inhibit");
    }
  else
    {
      g_clear_object (&session);

      screen_saver = org_freedesktop_screen_saver_proxy_new_sync (bus,
                                                                  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                                  "org.freedesktop.ScreenSaver",
                                                                  "/org/freedesktop/ScreenSaver",
                                                                  NULL,
                                                                  NULL);

      g_signal_connect (helper, "handle-inhibit", G_CALLBACK (handle_inhibit_fdo), NULL);
      g_debug ("Using org.freedesktop.ScreenSaver for inhibit");
    }


  if (!g_dbus_interface_skeleton_export (helper,
                                         bus,
                                         "/org/freedesktop/portal/desktop",
                                         error))
    return FALSE;

  g_debug ("providing %s", g_dbus_interface_skeleton_get_info (helper)->name);

  return TRUE;
}
