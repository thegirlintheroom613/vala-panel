#include "panel-layout.h"
#include "settings-manager.h"

ValaPanelCoreSettings *core_settings;
ValaPanelAppletManager *manager;

struct _ValaPanelAppletLayout
{
	GtkBox __parent__;
	const char *toplevel_id;
};

G_DEFINE_TYPE(ValaPanelAppletLayout, vala_panel_applet_layout, GTK_TYPE_BOX)

static void vala_panel_applet_layout_init(ValaPanelAppletLayout *self)
{
}

static void vala_panel_applet_layout_class_init(ValaPanelAppletLayoutClass *klass)
{
	manager       = vala_panel_applet_manager_new();
	core_settings = vala_panel_toplevel_get_core_settings();
}

ValaPanelAppletLayout *vala_panel_applet_layout_new(ValaPanelToplevel *top, GtkOrientation orient,
                                                    int spacing)
{
	return VALA_PANEL_APPLET_LAYOUT(g_object_new(vala_panel_applet_layout_get_type(),
	                                             "orientation",
	                                             orient,
	                                             "spacing",
	                                             spacing,
	                                             "baseline-position",
	                                             GTK_BASELINE_POSITION_CENTER,
	                                             "border-width",
	                                             0,
	                                             "hexpand",
	                                             true,
	                                             "vexpand",
	                                             true,
	                                             "toplevel-id",
	                                             vala_panel_toplevel_get_uuid(top),
	                                             NULL));
}

void vala_panel_applet_layout_init_applets(ValaPanelAppletLayout *self)
{
	g_auto(GStrv) core_units =
	    g_settings_get_strv(core_settings->core_settings, VALA_PANEL_CORE_UNITS);
	for (int i = 0; core_units[i] != NULL; i++)
	{
		const char *unit = core_units[i];
		ValaPanelUnitSettings *pl =
		    vala_panel_core_settings_get_by_uuid(core_settings, unit);
		if (!vala_panel_unit_settings_is_toplevel(pl))
		{
			g_autofree char *id =
			    g_settings_get_string(pl->default_settings, VALA_PANEL_TOPLEVEL_ID);
			g_autofree char *name =
			    g_settings_get_string(pl->default_settings, VALA_PANEL_KEY_NAME);
			if (!g_strcmp0(id, self->toplevel_id))
				vala_panel_applet_layout_place_applet(
				    self, vala_panel_applet_manager_applet_ref(manager, name), pl);
		}
	}
	vala_panel_applet_layout_update_applet_positions(self);
	return;
}

static void vala_panel_applet_on_destroy(ValaPanelApplet *self, void *data)
{
	ValaPanelAppletLayout *layout = VALA_PANEL_APPLET_LAYOUT(data);
	const char *uuid              = vala_panel_applet_get_uuid(self);
	vala_panel_applet_layout_applet_destroyed(layout, uuid);
	if (gtk_widget_in_destruction(GTK_WIDGET(layout)))
		vala_panel_core_settings_remove_unit_settings(core_settings, uuid);
}

void vala_panel_applet_layout_place_applet(ValaPanelAppletLayout *self, AppletInfoData *data,
                                           ValaPanelUnitSettings *s)
{
	ValaPanelApplet *applet =
	    vala_panel_applet_plugin_get_applet_widget(data->plugin,
	                                               VALA_PANEL_TOPLEVEL(gtk_widget_get_toplevel(
	                                                   GTK_WIDGET(self))),
	                                               s->custom_settings,
	                                               s->uuid);
	int position = g_settings_get_int(s->default_settings, VALA_PANEL_KEY_POSITION);
	gtk_box_pack_start(self, applet, false, true, 0);
	gtk_box_reorder_child(self, applet, position);
	if (vala_panel_applet_info_is_expandable(data->info))
	{
		g_settings_bind(s->default_settings,
		                VALA_PANEL_KEY_EXPAND,
		                applet,
		                "hexpand",
		                G_SETTINGS_BIND_GET);
		g_object_bind_property(applet, "hexpand", applet, "vexpand", G_BINDING_SYNC_CREATE);
	}
	g_signal_connect(applet, "destroy", G_CALLBACK(vala_panel_applet_on_destroy), self);
}

void vala_panel_applet_layout_applet_destroyed(ValaPanelAppletLayout *self, const char *uuid)
{
	ValaPanelUnitSettings *s = vala_panel_core_settings_get_by_uuid(core_settings, uuid);
	g_autofree char *name    = g_settings_get_string(s->default_settings, VALA_PANEL_KEY_NAME);
	vala_panel_applet_manager_applet_unref(manager, name);
}

void vala_panel_applet_layout_remove_applet(ValaPanelAppletLayout *self, ValaPanelApplet *applet)
{
	const char *uuid         = vala_panel_applet_get_uuid(applet);
	ValaPanelUnitSettings *s = vala_panel_core_settings_get_by_uuid(core_settings, uuid);
	gtk_widget_destroy(GTK_WIDGET(applet));
	vala_panel_core_settings_remove_unit_settings_full(core_settings, uuid, true);
}

const char *vala_panel_applet_layout_get_toplevel_id(ValaPanelAppletLayout *self)
{
	return self->toplevel_id;
}

GList *vala_panel_applet_layout_get_applets_list(ValaPanelAppletLayout *self)
{
	return gtk_container_get_children(GTK_CONTAINER(self));
}

ValaPanelUnitSettings *vala_panel_applet_layout_get_applet_settings(ValaPanelApplet *pl)
{
	const char *uuid = vala_panel_applet_get_uuid(pl);
	return vala_panel_core_settings_get_by_uuid(core_settings, uuid);
}