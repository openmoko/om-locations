/* e_nav_item_location.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "e_nav.h"
#include "e_nav_theme.h"
#include "e_nav_item_location.h"
#include "e_nav_item_neo_me.h"
#include "e_flyingmenu.h"
#include "widgets/e_nav_dialog.h"
#include "widgets/e_nav_alert.h"
#include "widgets/e_nav_contact_editor.h"
#include "e_ctrl.h"
#include <time.h>
#include <ctype.h>

#define LOCATION_TITLE_LEN   40
#define LOCATION_MESSAGE_LEN 80
#define LOCATION_DESCRIPTION_LEN (LOCATION_TITLE_LEN + 1 + LOCATION_MESSAGE_LEN)

static char *get_time_diff_string(time_t time_then);

typedef struct _Location_Data Location_Data;
static Evas_Object *xxx_ctrl;

struct _Location_Data
{
   Diversity_Tag          *tag;
   time_t                  timestamp;
   uint                    unread;
   const char             *name;
   const char             *note;

   unsigned char           visible : 1;
   unsigned char           details : 1;
   unsigned char           timestamp_changed : 1;
   char                   *timestring;
};

static int
form_description(char *buf, int len, const char *title, const char *message)
{
   return snprintf(buf, len, "%s\n%s", title, message);
}

static int
location_save(Evas_Object *item, const char *title, const char *msg)
{
   Location_Data *locd;
   char desc[LOCATION_DESCRIPTION_LEN + 1];

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd)
     return FALSE;

   form_description(desc, sizeof(desc), title, msg);

   if (diversity_tag_prop_set(locd->tag, "description", desc))
     {
        if (locd->name) evas_stringshare_del(locd->name);
        if (title) locd->name = evas_stringshare_add(title);
        else locd->name = NULL;

        if (locd->note) evas_stringshare_del(locd->note);
        if (msg) locd->note = evas_stringshare_add(msg);
        else locd->note = NULL;

        e_ctrl_taglist_tag_set(xxx_ctrl, item);
        e_nav_world_item_location_name_set(item, title);

	return TRUE;
     }
   else
     {
	printf("failed to edit tag %s\n", title);

	return FALSE;
     }
}

static int
is_phone_number(const char *input)
{
   int length;
   int i;
   if(input)
     length = strlen(input);

   if(!input || length<3)
     return FALSE;

   if(input[0]!='+' && !isdigit(input[0]))
     return FALSE;

   for(i=1; i<length; i++)
     if(!isdigit(input[i]))
       return FALSE;

   return TRUE;
}

static const char *
trim_leading_space(const char *str)
{
   const char *buf;
   if(!str) return str;
   buf = str;
   while(buf && isspace(*buf) )
     buf++;

   if(str != buf)
     return buf;
   else
     return str;
}

static Diversity_Equipment *
get_phone_equip(Evas_Object *nav)
{
   Diversity_Equipment *eqp;
   Evas_Object *neo_me;

   if (!nav)
     return NULL;

   neo_me = e_nav_world_neo_me_get(nav);
   if (!neo_me)
     return NULL;

   eqp = e_nav_world_item_neo_me_equipment_get(neo_me, "qtopia");
   if (!eqp)
     eqp = e_nav_world_item_neo_me_equipment_get(neo_me, "phonekit");

   return eqp;
}

static const char *
location_send(Evas_Object *item, const char *to)
{
   Location_Data *locd;
   Diversity_Equipment *eqp;
   Evas_Object *bard;
   const char *error = NULL;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   eqp = get_phone_equip(e_nav_world_item_nav_get(item));

   if (!locd || !to || !eqp)
     return _("Unable to send SMS");

   bard = e_ctrl_contact_get_by_name(xxx_ctrl, to);
   if (!bard)
     {
	to = trim_leading_space(to);
	bard = e_ctrl_contact_get_by_name(xxx_ctrl, to);
	if (!bard)
	  bard = e_ctrl_contact_get_by_number(xxx_ctrl, to);
     }

   if (bard || is_phone_number(to))
     {
	int ok;

	if (bard)
	  ok = diversity_sms_tag_share((Diversity_Sms *) eqp,
		e_nav_world_item_neo_other_bard_get(bard),
		locd->tag);
	else
	  ok = diversity_sms_tag_send((Diversity_Sms *) eqp,
		to, locd->tag);

	if (!ok)
	  error = _("Send tag failed");
     }
   else
     {
	error = _("Contact not found");
     }

   return error;
}

static void
_e_nav_world_item_cb_menu_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *location_object = (Evas_Object *) data;

   e_flyingmenu_deactivate(obj);
   e_nav_world_item_location_action_edit(location_object);
}

static void
_e_nav_world_item_cb_menu_2(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *location_object = (Evas_Object *) data;

   e_flyingmenu_deactivate(obj);
   e_nav_world_item_location_action_send(location_object);
}

static void
cb_menu_activate(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Evas_Object *om;
   om = e_flyingmenu_add(evas_object_evas_get(obj));
   e_flyingmenu_theme_source_set(om, data);  // data is THEMEDIR
   e_flyingmenu_autodelete_set(om, 1);
   e_flyingmenu_source_object_set(om, obj);   // obj is location evas object
   /* FIXME: real menu items */
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 160, _("edit"),
			       _e_nav_world_item_cb_menu_1, obj);
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 160, _("send"),
			       _e_nav_world_item_cb_menu_2, obj);
   evas_object_show(om);
   e_flyingmenu_activate(om);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *item;
   Location_Data *locd;

   item = (Evas_Object *) data;
   if (!item)
     return;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd)
     return;

   e_nav_world_item_location_details_set(item, !locd->details);
}

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Location_Data *locd;

   if(!obj) return;

   e_ctrl_taglist_tag_delete(xxx_ctrl, obj);

   locd = evas_object_data_get(obj, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->name) evas_stringshare_del(locd->name);
   if (locd->note) evas_stringshare_del(locd->note);
   if (locd->timestring) free(locd->timestring);

   edje_object_signal_callback_del(obj, "MENU_ACTIVATE", "e.text.name", cb_menu_activate);
   free(locd);
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_location_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Object *tag)
{
   Evas_Object *o;
   Location_Data *locd;
   int x, y, w, h;

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   locd = calloc(1, sizeof(Location_Data));
   if (!locd) return NULL;
   locd->tag = (Diversity_Tag *) tag;
   o = e_nav_theme_object_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/location");
   edje_object_part_text_set(o, "e.text.name", _("No Title"));
   edje_object_signal_callback_add(o, "MENU_ACTIVATE", "e.text.name", cb_menu_activate, (void *)theme_dir);

   evas_object_event_callback_add(edje_object_part_object_get(o, "e.image.location"),
                                  EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_world_item_cb_mouse_down,
				  o);
   evas_object_geometry_get(edje_object_part_object_get(o, "e.image.location"), &x, &y, &w, &h);
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(o, lon, lat, w, h);
   e_nav_world_item_scale_set(o, 0);
   e_nav_world_item_update(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_del, NULL);
   evas_object_data_set(o, "nav_world_item_location_data", locd);
   evas_object_show(o);

   if (!xxx_ctrl)
     xxx_ctrl = e_nav_world_ctrl_get(nav);

   return o;
}

void
e_nav_world_item_location_name_set(Evas_Object *item, const char *name)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->name) evas_stringshare_del(locd->name);
   if (name) locd->name = evas_stringshare_add(name);
   else locd->name = NULL;
   if(!locd->name || !strcmp(locd->name, ""))
     edje_object_part_text_set(item, "e.text.name", _("No Title"));
   else
     edje_object_part_text_set(item, "e.text.name", locd->name);
}

const char *
e_nav_world_item_location_name_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return NULL;
   return locd->name;
}

void
e_nav_world_item_location_note_set(Evas_Object *item, const char *note)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->note) evas_stringshare_del(locd->note);
   if (note) locd->note = evas_stringshare_add(note);
   else locd->note = NULL;
}

const char *
e_nav_world_item_location_note_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return NULL;
   return locd->note;
}

void
e_nav_world_item_location_visible_set(Evas_Object *item, Evas_Bool visible)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if ((visible && locd->visible) || ((!visible) && (!locd->visible))) return;
   locd->visible = visible;
   if (locd->visible)
     edje_object_signal_emit(item, "e,state,visible", "e");
   else
     edje_object_signal_emit(item, "e,state,invisible", "e");
}

Evas_Bool
e_nav_world_item_location_visible_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->visible;
}

void
e_nav_world_item_location_unread_set(Evas_Object *item, uint unread)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   locd->unread = unread;
}

int
e_nav_world_item_location_unread_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->unread;
}

Diversity_Tag *
e_nav_world_item_location_tag_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->tag;
}

void
e_nav_world_item_location_timestamp_set(Evas_Object *item, time_t secs)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;

   locd->timestamp = secs;
   locd->timestamp_changed = TRUE;
}

int
e_nav_world_item_location_timestamp_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->timestamp;
}

const char *
e_nav_world_item_location_timestring_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd)
     return NULL;

   if (locd->timestamp_changed)
     {
	char *str;

	if (locd->timestring)
	  free(locd->timestring);

	str = get_time_diff_string(locd->timestamp);
	if (str)
	  {
	     locd->timestring = str;
	     edje_object_part_text_set(item, "e.text.name2", str);
	  }
	else
	  {
	     locd->timestring = NULL;
	  }

	locd->timestamp_changed = FALSE;
     }

   return locd->timestring;
}

void
e_nav_world_item_location_details_set(Evas_Object *item, Evas_Bool active)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd)
     return;

   active = !!active;

   if (locd->details != active)
     {
	locd->details = active;

	if (locd->details)
	  {
	     if (locd->timestamp_changed)
	       e_nav_world_item_location_timestring_get(item);

	     edje_object_signal_emit(item, "e,state,active", "e");
	  }
	else
	  {
	     edje_object_signal_emit(item, "e,state,passive", "e");
	  }
     }
}

Evas_Bool
e_nav_world_item_location_details_get(Evas_Object *item)
{
   Location_Data *locd;

   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd)
     return FALSE;

   return locd->details;
}

void
e_nav_world_item_location_title_show(Evas_Object *item)
{
   e_nav_world_item_location_details_set(item, TRUE);
}

static char *
get_time_diff_string(time_t time_then)
{
   char time_diff_string[PATH_MAX];
   time_t time_now, time_diff;
   int today_secs;
   struct tm now, then;
   struct tm *now_p, *then_p;
   int age;

   time(&time_now);
   now_p = localtime(&time_now);
   memcpy(&now, now_p, sizeof(now));

   then_p = localtime(&time_then);
   memcpy(&then, then_p, sizeof(then));

   if(time_then > time_now)
     {
        snprintf(time_diff_string, sizeof(time_diff_string),
                 "%s", ctime(&time_then));
     }

   if(now.tm_year != then.tm_year)
     {
	age = now.tm_year - then.tm_year;

	if (age == 1)
	  snprintf(time_diff_string, sizeof(time_diff_string),
		_("Last year"));
        else
	  snprintf(time_diff_string, sizeof(time_diff_string),
		ngettext("One year ago", "%d years ago", age), age);

        return strdup(time_diff_string);
     }
   else if(now.tm_mon != then.tm_mon)
     {
	age = now.tm_mon - then.tm_mon;

        if (age == 1)
	  snprintf(time_diff_string, sizeof(time_diff_string),
		_("Last month"));
        else
	  snprintf(time_diff_string, sizeof(time_diff_string),
		ngettext("One month ago", "%d months ago", age), age);

        return strdup(time_diff_string);
     }
   else
     {
            today_secs = (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec;
            time_diff = time_now - time_then;

            if (time_diff >= today_secs)
	      {
		 age = 1 + (time_diff - today_secs) / 86400;
		 if (age == 1)
		   {
		      snprintf(time_diff_string, sizeof(time_diff_string),
			    _("Yesterday"));
		   }
		 else if (age < 7)
		   {
		      snprintf(time_diff_string, sizeof(time_diff_string),
			    ngettext("One day ago", "%d days ago", age), age);
		   }
		 else
		   {
		      age /= 7;

		      if (age == 1)
			snprintf(time_diff_string, sizeof(time_diff_string),
			      _("Last week"));
		      else
			snprintf(time_diff_string, sizeof(time_diff_string),
			      ngettext("One week ago", "%d weeks ago", age), age);
		   }

		 return strdup(time_diff_string);
	      }
            else
              {
                 return strdup(_("Today"));
              }
     }
}

/*********************************************************/
/*********************************************************/
enum {
     ACTION_STATE_INIT,
     ACTION_STATE_CREATE,
     ACTION_STATE_EDIT,
     ACTION_STATE_DELETE,
     ACTION_STATE_SEND,
     ACTION_STATE_EDITOR,
     ACTION_STATE_REPORT,
     ACTION_STATE_END,
     N_ACTION_STATES
};

typedef struct _Action_Data {
     int state;

     Evas_Object *nav;
     Evas *evas;

     Evas_Object *dialog;
     Evas_Object *alert;
     Evas_Object *editor;

     Diversity_World *world;

     Evas_Object *loc;
     double lon, lat;
     char *send_error;
} Action_Data;

static void action_next(Action_Data *act_data, int state);

static void
action_to_end(Action_Data *act_data)
{
   action_next(act_data, ACTION_STATE_END);
}

static void
action_to_editor(Action_Data *act_data)
{
   action_next(act_data, ACTION_STATE_EDITOR);
}

static void
action_to_delete(Action_Data *act_data)
{
   action_next(act_data, ACTION_STATE_DELETE);
}

static void
action_finish(Action_Data *act_data)
{
   if (act_data->dialog)
     {
	e_dialog_deactivate(act_data->dialog);
	act_data->dialog = NULL;
     }

   if (act_data->alert)
     {
	e_alert_deactivate(act_data->alert);
	act_data->alert = NULL;
     }

   if (act_data->editor)
     {
	evas_object_del(act_data->editor);
	act_data->editor = NULL;
     }

   if (act_data->send_error)
     free(act_data->send_error);

   free(act_data);
}

static void
action_create(Action_Data *act_data)
{
   const char *title = NULL, *msg;
   Diversity_Tag *tag = NULL;

   if (act_data->dialog)
     {
	char desc[LOCATION_DESCRIPTION_LEN + 1];

	title = e_dialog_textblock_text_get(act_data->dialog,
	      _("Edit title"));
	msg = e_dialog_textblock_text_get(act_data->dialog,
	      _("Edit message"));
	form_description(desc, sizeof(desc), title, msg);

	tag = diversity_world_tag_add(act_data->world,
	      act_data->lon, act_data->lat, desc);
     }

   if (!tag)
     printf("failed to add tag %s\n", title);

   action_next(act_data, ACTION_STATE_END);
}

static void
action_save(Action_Data *act_data)
{
   const char *title, *msg;

   if (act_data->dialog)
     {
	title = e_dialog_textblock_text_get(act_data->dialog,
	      _("Edit title"));
	msg = e_dialog_textblock_text_get(act_data->dialog,
	      _("Edit message"));

	location_save(act_data->loc, title, msg);
     }

   action_next(act_data, ACTION_STATE_END);
}

static void
action_send(Action_Data *act_data)
{
   if (act_data->send_error)
     {
	free(act_data->send_error);
	act_data->send_error = NULL;
     }

   if (act_data->editor)
     {
	const char *to, *error;

	to = e_contact_editor_input_get(act_data->editor);
	error = location_send(act_data->loc, to);
	if (error)
	  act_data->send_error = strdup(error);
     }
   else
     {
	act_data->send_error = strdup(_("No contact selected"));
     }

   action_next(act_data, ACTION_STATE_REPORT);
}

static void
action_delete(Action_Data *act_data)
{
   Location_Data *locd;

   locd = evas_object_data_get(act_data->loc, "nav_world_item_location_data");
   if (locd)
     {
	if (diversity_world_tag_remove(act_data->world, locd->tag))
	  locd->tag = NULL;
	else
	  printf("failed to delete tag %s\n", locd->name);
     }

   action_next(act_data, ACTION_STATE_END);
}

static void
action_show_dialog(Action_Data *act_data)
{
   Evas_Object *od;
   const char *title = NULL, *msg = NULL;

   od = act_data->dialog;
   if (!od)
     {
	od = e_dialog_add(act_data->evas);
	if (!od)
	  {
	     action_next(act_data, ACTION_STATE_END);

	     return;
	  }

	e_dialog_theme_source_set(od, THEMEDIR);
	e_dialog_transient_for_set(od, act_data->nav);

	act_data->dialog = od;
     }

   switch (act_data->state)
     {
      case ACTION_STATE_CREATE:
	 title = _("Save your location");
	 msg = _("Save your current location");
	 break;
      case ACTION_STATE_EDIT:
	 title = _("Edit your location");
	 msg = _("Press the text boxes to edit this location.");
	 break;
      case ACTION_STATE_SEND:
	 title = _("Send your location");
	 msg = _("Send your favorite location to a friend by SMS.");
	 break;
      default:
	 action_next(act_data, ACTION_STATE_END);
	 return;

	 break;
     }

   e_dialog_title_set(act_data->dialog, title, msg);

   if (act_data->loc)
     {
	title = e_nav_world_item_location_name_get(act_data->loc);
	msg = e_nav_world_item_location_note_get(act_data->loc);
     }
   else
     {
	title = NULL;
	msg = NULL;
     }

   e_dialog_textblock_add(act_data->dialog, _("Edit title"), title,
	 40, LOCATION_TITLE_LEN, NULL);
   e_dialog_textblock_add(act_data->dialog, _("Edit message"), msg,
	 100, LOCATION_MESSAGE_LEN, NULL);

   switch (act_data->state)
     {
      case ACTION_STATE_CREATE:
	 e_dialog_button_add(act_data->dialog, _("Save"),
	       (void *) action_create, act_data);
	 e_dialog_button_add(act_data->dialog, _("Cancel"),
	       (void *) action_to_end, act_data);
	 break;
      case ACTION_STATE_EDIT:
	 e_dialog_button_add(act_data->dialog, _("Save"),
	       (void *) action_save, act_data);
	 e_dialog_button_add(act_data->dialog, _("Cancel"),
	       (void *) action_to_end, act_data);
	 e_dialog_button_add(act_data->dialog, _("Delete"),
	       (void *) action_to_delete, act_data);
	 break;
      case ACTION_STATE_SEND:
	 e_dialog_button_add(act_data->dialog, _("Send"),
	       (void *) action_to_editor, act_data);
	 e_dialog_button_add(act_data->dialog, _("Cancel"),
	       (void *) action_to_end, act_data);
	 break;
     }

   e_dialog_activate(act_data->dialog);
   evas_object_show(act_data->dialog);
}

static void
action_show_editor(Action_Data *act_data)
{
   Evas_Object *editor;

   if (act_data->dialog && act_data->loc)
     {
	const char *title, *msg;

	title = e_dialog_textblock_text_get(act_data->dialog,
	      _("Edit title"));
	msg = e_dialog_textblock_text_get(act_data->dialog,
	      _("Edit message"));

	location_save(act_data->loc, title, msg);
     }

   editor = act_data->editor;
   if (!editor)
     {
	Evas_Coord x, y, w, h;
	Evas_List *contacts;

	editor = e_contact_editor_add(act_data->evas);
	if (!editor)
	  {
	     action_next(act_data, ACTION_STATE_END);

	     return;
	  }

	e_contact_editor_theme_source_set(editor, THEMEDIR,
	      (void *) action_send, act_data,
	      (void *) action_to_end, act_data);

	evas_output_viewport_get(act_data->evas, &x, &y, &w, &h);
	evas_object_move(editor, x, y);
	evas_object_resize(editor, w, h);

	e_contact_editor_input_set(editor, _("To:"), NULL);

	contacts = e_ctrl_contact_list(xxx_ctrl);
	e_contact_editor_contacts_set(editor, contacts);

	act_data->editor = editor;
     }

   evas_object_show(editor);
}

static void
action_show_alert(Action_Data *act_data)
{
   Evas_Object *oa = act_data->alert;

   if (!oa)
     {
	oa = e_alert_add(act_data->evas);
	if (!oa)
	  {
	     action_next(act_data, ACTION_STATE_END);

	     return;
	  }

	e_alert_theme_source_set(oa, THEMEDIR);
	e_alert_transient_for_set(oa, act_data->nav);

	act_data->alert = oa;
     }

   switch (act_data->state)
     {
      case ACTION_STATE_DELETE:
	 e_alert_title_set(oa, _("DELETE"), _("Are you sure?"));
	 e_alert_title_color_set(oa, 255, 0, 0, 255);

	 e_alert_button_add(oa, _("Yes"),
	       (void *) action_delete, act_data);
	 e_alert_button_add(oa, _("No"),
	       (void *) action_to_end, act_data);
	 break;
      case ACTION_STATE_REPORT:
      default:
	 if (act_data->send_error)
	   {
	      e_alert_title_set(oa, _("ERROR"), act_data->send_error);
	      e_alert_title_color_set(oa, 255, 0, 0, 255);
	   }
	 else
	   {
	      e_alert_title_set(oa, _("SUCCESS"), _("Tag sent"));
	      e_alert_title_color_set(oa, 0, 255, 0, 255);
	   }

	 e_alert_button_add(oa, _("OK"),
	       (void *) action_to_end, act_data);
	 break;
     }

   e_alert_activate(oa);
   evas_object_show(oa);
}

static int
action_next_check(Action_Data *act_data, int state)
{
   int valid = TRUE;

   switch (act_data->state)
     {
      case ACTION_STATE_INIT:
	 switch (state)
	   {
	    case ACTION_STATE_CREATE:
	    case ACTION_STATE_EDIT:
	    case ACTION_STATE_SEND:
	       break;
	    default:
	       valid = FALSE;
	       break;
	   }
	 break;
      case ACTION_STATE_CREATE:
	 if (state != ACTION_STATE_END)
	   valid = FALSE;
	 break;
      case ACTION_STATE_EDIT:
	 switch (state)
	   {
	    case ACTION_STATE_DELETE:
	    case ACTION_STATE_END:
	       break;
	    default:
	       valid = FALSE;
	       break;
	   }
	 break;
      case ACTION_STATE_DELETE:
	 if (state != ACTION_STATE_END)
	   valid = FALSE;
	 break;
      case ACTION_STATE_SEND:
	 switch (state)
	   {
	    case ACTION_STATE_EDITOR:
	    case ACTION_STATE_END:
	       break;
	    default:
	       valid = FALSE;
	       break;
	   }
	 break;
      case ACTION_STATE_EDITOR:
	 switch (state)
	   {
	    case ACTION_STATE_REPORT:
	    case ACTION_STATE_END:
	       break;
	    default:
	       valid = FALSE;
	       break;
	   }
	 break;
      case ACTION_STATE_REPORT:
	 if (state != ACTION_STATE_END)
	   valid = FALSE;
	 break;
      case ACTION_STATE_END:
      default:
	 valid = FALSE;
	 break;
     }

   return valid;
}

static void
action_next(Action_Data *act_data, int state)
{
   if (!action_next_check(act_data, state))
     {
	printf("action state %d -> %d\n", act_data->state, state);
	state = ACTION_STATE_END;
     }

   if (act_data->dialog)
     evas_object_hide(act_data->dialog);
   if (act_data->alert)
     evas_object_hide(act_data->alert);
   if (act_data->editor)
     evas_object_hide(act_data->editor);

   act_data->state = state;

   switch (state)
     {
      case ACTION_STATE_CREATE:
      case ACTION_STATE_EDIT:
      case ACTION_STATE_SEND:
	 action_show_dialog(act_data);
	 break;
      case ACTION_STATE_EDITOR:
	 action_show_editor(act_data);
	 break;
      case ACTION_STATE_DELETE:
      case ACTION_STATE_REPORT:
	 action_show_alert(act_data);
	 break;
      case ACTION_STATE_END:
      default:
	 action_finish(act_data);
	 break;
     }
}

static Action_Data *
action_new(Evas_Object *nav)
{
   Action_Data *act_data;

   act_data = calloc(sizeof(*act_data), 1);
   if (!act_data)
     return NULL;

   act_data->state = ACTION_STATE_INIT;
   act_data->nav = nav;
   act_data->evas = evas_object_evas_get(nav);
   act_data->world = e_nav_world_get(nav);
   if (!act_data->world)
     {
	free(act_data);

	return NULL;
     }

   return act_data;
}

void
e_nav_world_item_location_action_new(Evas_Object *nav, double lon, double lat)
{
   Action_Data *act_data;

   act_data = action_new(nav);
   if (!act_data)
     return;

   act_data->lon = lon;
   act_data->lat = lat;

   action_next(act_data, ACTION_STATE_CREATE);
}

void
e_nav_world_item_location_action_edit(Evas_Object *item)
{
   Action_Data *act_data;

   act_data = action_new(e_nav_world_item_nav_get(item));
   if (!act_data)
     return;

   act_data->loc = item;

   action_next(act_data, ACTION_STATE_EDIT);
}

void
e_nav_world_item_location_action_send(Evas_Object *item)
{
   Action_Data *act_data;

   act_data = action_new(e_nav_world_item_nav_get(item));
   if (!act_data)
     return;

   act_data->loc = item;

   action_next(act_data, ACTION_STATE_SEND);
}
