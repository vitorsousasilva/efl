#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#define ELM_WIDGET_ITEM_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_multibuttonentry.h"

#define MY_CLASS ELM_MULTIBUTTONENTRY_CLASS

#define MY_CLASS_NAME "Elm_Multibuttonentry"
#define MY_CLASS_NAME_LEGACY "elm_multibuttonentry"

#define MAX_STR     256
#define MIN_W_ENTRY 10

//widget signals
static const char SIG_ITEM_SELECTED[] = "item,selected";
static const char SIG_ITEM_ADDED[] = "item,added";
static const char SIG_ITEM_DELETED[] = "item,deleted";
static const char SIG_ITEM_CLICKED[] = "item,clicked";
static const char SIG_ITEM_LONGPRESSED[] = "item,longpressed";
static const char SIG_CLICKED[] = "clicked";
static const char SIG_FOCUSED[] = "focused";
static const char SIG_UNFOCUSED[] = "unfocused";
static const char SIG_EXPANDED[] = "expanded";
static const char SIG_CONTRACTED[] = "contracted";
static const char SIG_EXPAND_STATE_CHANGED[] = "expand,state,changed";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_ITEM_SELECTED, ""},
   {SIG_ITEM_ADDED, ""},
   {SIG_ITEM_DELETED, ""},
   {SIG_ITEM_CLICKED, ""},
   {SIG_ITEM_LONGPRESSED, ""},
   {SIG_CLICKED, ""},
   {SIG_FOCUSED, ""},
   {SIG_UNFOCUSED, ""},
   {SIG_EXPANDED, ""},
   {SIG_CONTRACTED, ""},
   {SIG_EXPAND_STATE_CHANGED, ""},
   {NULL, NULL}
};

EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_widget_translate(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   Elm_Object_Item *it;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->items, l, it)
     eo_do(it, elm_wdg_item_translate());

   eo_do_super(obj, MY_CLASS, elm_obj_widget_translate());

   return EINA_TRUE;
}

static char *
_format_count(int count, void *data EINA_UNUSED)
{
   char buf[32];

   if (!snprintf(buf, sizeof(buf), "... + %d", count)) return NULL;
   return strdup(buf);
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_widget_theme_apply(Eo *obj, Elm_Multibuttonentry_Data *sd)
{
   Eina_List *l;
   Elm_Object_Item *eo_item;

   Eina_Bool int_ret = EINA_FALSE;
   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_theme_apply());
   if (!int_ret) return EINA_FALSE;

   EINA_LIST_FOREACH(sd->items, l, eo_item)
     {
        ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
        if (VIEW(item))
          elm_layout_theme_set
            (VIEW(item), "multibuttonentry", "btn", elm_widget_style_get(obj));
        elm_object_scale_set(VIEW(item), elm_widget_scale_get(obj) * elm_config_scale_get());
     }

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static void
_visual_guide_text_set(Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   elm_box_unpack(sd->box, sd->guide_text);
   elm_box_unpack(sd->box, sd->entry);
   if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK) return;

   if (!elm_widget_focus_get(obj))
     elm_object_focus_set(sd->entry, EINA_FALSE);

   if ((!eina_list_count(sd->items)) && sd->guide_text
       && (!elm_widget_focus_get(obj)) && (!sd->n_str))
     {
        evas_object_hide(sd->entry);
        elm_box_pack_end(sd->box, sd->guide_text);
        evas_object_show(sd->guide_text);
        sd->view_state = MULTIBUTTONENTRY_VIEW_GUIDETEXT;
     }
   else
     {
        evas_object_hide(sd->guide_text);

        if (sd->editable)
          {
             elm_box_pack_end(sd->box, sd->entry);
             evas_object_show(sd->entry);
             if (elm_widget_focus_get(obj))
               {
                  if (!sd->selected_it)
                    elm_object_focus_set(sd->entry, EINA_TRUE);
               }
          }
        sd->view_state = MULTIBUTTONENTRY_VIEW_ENTRY;
     }
}

static void
_shrink_mode_set(Evas_Object *obj,
                 Eina_Bool shrink)
{
   int count;
   Eina_List *l;
   Elm_Object_Item *eo_item;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   if (sd->view_state == MULTIBUTTONENTRY_VIEW_ENTRY)
     evas_object_hide(sd->entry);
   else if (sd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
     evas_object_hide(sd->guide_text);
   else if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
     {
        evas_object_hide(sd->end);
        sd->view_state = MULTIBUTTONENTRY_VIEW_NONE;
     }

   if (shrink == EINA_TRUE)
     {
        Evas_Coord w = 0, w_tmp = 0;
        Evas_Coord box_inner_item_width_padding = 0;

        elm_box_padding_get(sd->box, &box_inner_item_width_padding, NULL);
        // unpack all items and entry
        elm_box_unpack_all(sd->box);
        EINA_LIST_FOREACH(sd->items, l, eo_item)
          {
             ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
             evas_object_hide(VIEW(item));
             item->visible = EINA_FALSE;
          }
        // pack buttons only 1line
        w = sd->w_box;

        if (sd->label)
          {
             elm_box_pack_end(sd->box, sd->label);
             evas_object_size_hint_min_get(sd->label, &w_tmp, NULL);
             w -= w_tmp;
             w -= box_inner_item_width_padding;
          }

        eo_item = NULL;
        count = eina_list_count(sd->items);

        EINA_LIST_FOREACH(sd->items, l, eo_item)
          {
             Evas_Coord item_w, w_label_count = 0, h = 0;
             char *buf;

             ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
             elm_box_pack_end(sd->box, VIEW(item));
             evas_object_show(VIEW(item));
             item->visible = EINA_TRUE;

             evas_object_size_hint_min_get(VIEW(item), &item_w, NULL);

             w -= item_w;
             w -= box_inner_item_width_padding;
             count--;

             if (count > 0)
               {
                  buf = sd->format_func(count, (void *)sd->format_func_data);
                  if (buf)
                    {
                       edje_object_part_text_escaped_set
                           (sd->end, "elm.text", buf);
                       free(buf);
                    }

                  edje_object_size_min_calc(sd->end, &w_label_count, NULL);
                  elm_coords_finger_size_adjust(1, &w_label_count, 1, NULL);
               }

             if ((w < 0) || (w < w_label_count))
               {
                  elm_box_unpack(sd->box, VIEW(item));
                  evas_object_hide(VIEW(item));
                  item->visible = EINA_FALSE;
                  count++;

                  buf = sd->format_func(count, (void *)sd->format_func_data);
                  if (buf)
                    {
                       edje_object_part_text_escaped_set
                           (sd->end, "elm.text", buf);
                       free(buf);
                    }

                  edje_object_size_min_calc(sd->end, &w_label_count, &h);
                  elm_coords_finger_size_adjust(1, &w_label_count, 1, &h);
                  evas_object_size_hint_min_set
                    (sd->end, w_label_count, h);
                  elm_box_pack_end(sd->box, sd->end);
                  evas_object_show(sd->end);

                  sd->view_state = MULTIBUTTONENTRY_VIEW_SHRINK;
                  evas_object_smart_callback_call
                    (obj, SIG_EXPAND_STATE_CHANGED, (void *)1);
                  break;
               }
          }
     }
   else
     {
        // unpack all items and entry
        elm_box_unpack_all(sd->box);
        EINA_LIST_FOREACH(sd->items, l, eo_item)
          {
             ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
             evas_object_hide(VIEW(item));
             item->visible = EINA_FALSE;
          }
        evas_object_hide(sd->end);

        // pack buttons only 1line

        if (sd->label) elm_box_pack_end(sd->box, sd->label);

        // pack remain btns
        eo_item = NULL;
        EINA_LIST_FOREACH(sd->items, l, eo_item)
          {
             ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
             elm_box_pack_end(sd->box, VIEW(item));
             evas_object_show(VIEW(item));
             item->visible = EINA_TRUE;
          }

        sd->view_state = MULTIBUTTONENTRY_VIEW_NONE;
        evas_object_smart_callback_call
          (obj, SIG_EXPAND_STATE_CHANGED, (void *)(uintptr_t)sd->shrink);
     }

   if (sd->view_state != MULTIBUTTONENTRY_VIEW_SHRINK)
     _visual_guide_text_set(obj);
}

static void
_view_update(Elm_Multibuttonentry_Data *sd)
{
   Evas_Coord width = 1, height = 1;
   Evas_Object *obj = sd->parent;

   if (sd->w_box <= 0) return;

   // update label
   if (sd->label)
     {
        elm_box_unpack(sd->box, sd->label);
        elm_box_pack_start(sd->box, sd->label);
        evas_object_size_hint_min_get(sd->label, &width, &height);
     }

   if (sd->guide_text)
     {
        Evas_Coord guide_text_width = sd->w_box - width;

        evas_object_size_hint_min_set(sd->guide_text, guide_text_width, height);
     }

   // update buttons in shrink mode
   if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
     _shrink_mode_set(obj, EINA_TRUE);

   // update guidetext
   _visual_guide_text_set(obj);
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_widget_on_focus(Eo *obj, Elm_Multibuttonentry_Data *sd)
{
   if (elm_widget_focus_get(obj))
     {
        // ACCESS
        if (_elm_config->access_mode == ELM_ACCESS_MODE_ON) goto end;

        if (sd->editable)
          {
             if ((sd->selected_it))
               {
                  elm_entry_input_panel_show(sd->entry);
               }
             else if (((!sd->selected_it) || (!eina_list_count(sd->items))))
               {
                  elm_entry_cursor_end_set(sd->entry);
                  _view_update(sd);
                  elm_entry_input_panel_show(sd->entry);
               }
          }
        evas_object_smart_callback_call(obj, SIG_FOCUSED, NULL);
     }
   else
     {
        _view_update(sd);

        elm_entry_input_panel_hide(sd->entry);
        evas_object_smart_callback_call(obj, SIG_UNFOCUSED, NULL);
     }

end:
   return EINA_TRUE;
}

static void
_item_del(Elm_Multibuttonentry_Item_Data *item)
{
   Eina_List *l;
   Elm_Object_Item *eo_it;
   Evas_Object *obj = WIDGET(item);

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   EINA_LIST_FOREACH(sd->items, l, eo_it)
     {
        ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_it, it);
        if (it == item)
          {
             sd->items = eina_list_remove(sd->items, eo_it);
             elm_box_unpack(sd->box, VIEW(it));

             evas_object_smart_callback_call(obj, SIG_ITEM_DELETED, eo_it);

             evas_object_del(VIEW(it));

             if (sd->selected_it == it)
               sd->selected_it = NULL;
             break;
          }
     }

   if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
     _shrink_mode_set(obj, EINA_TRUE);

   if (!eina_list_count(sd->items))
     _visual_guide_text_set(obj);
}

static void
_current_item_state_change(Evas_Object *obj,
                             Multibuttonentry_Button_State state)
{
   Elm_Multibuttonentry_Item_Data *item;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   item = sd->selected_it;

   if (!item) return;

   switch (state)
     {
      case MULTIBUTTONENTRY_BUTTON_STATE_SELECTED:
        elm_layout_signal_emit(VIEW(item), "elm,state,focused", "elm");
        evas_object_smart_callback_call(obj, SIG_ITEM_SELECTED, EO_OBJ(item));
        break;
      case MULTIBUTTONENTRY_BUTTON_STATE_DEFAULT:
      default:
        elm_layout_signal_emit(VIEW(item), "elm,state,default", "elm");
        sd->selected_it = NULL;
        break;
     }
}

static void
_current_item_change(Evas_Object *obj,
                       Elm_Object_Item *eo_it)
{
   Eina_List *l;
   Elm_Object_Item *eo_temp_it;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);
   ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_it, it);

   // change the state of previous item to "default"
   _current_item_state_change(obj, MULTIBUTTONENTRY_BUTTON_STATE_DEFAULT);

   // change the current
   EINA_LIST_FOREACH(sd->items, l, eo_temp_it)
     {
        if (eo_temp_it == eo_it)
          {
             sd->selected_it = it;
             break;
          }
     }
   // change the state of current item to "focused"
   _current_item_state_change(obj, MULTIBUTTONENTRY_BUTTON_STATE_SELECTED);
}

static void
_item_select(Evas_Object *obj,
               Elm_Multibuttonentry_Item_Data *it)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   if (it)
     {
        _current_item_change(obj, EO_OBJ(it));
        if (elm_widget_focus_get(obj))
          {
             elm_object_focus_set(sd->entry, EINA_FALSE);
             evas_object_focus_set(VIEW(it), EINA_TRUE);

             // ACCESS
             if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
               {
                  Evas_Object *ao, *po;
                  Eina_Strbuf *buf;
                  const char *part, *text;

                  part = "elm.btn.text";
                  po = (Evas_Object *)edje_object_part_object_get(elm_layout_edje_get(VIEW(it)), part);
                  ao = evas_object_data_get(po, "_part_access_obj");
                  _elm_access_highlight_set(ao);

                  buf = eina_strbuf_new();
                  eina_strbuf_append_printf(buf,
                    "multi button entry item %s is selected",
                    edje_object_part_text_get(elm_layout_edje_get(VIEW(it)), part));

                  text = (const char*)eina_strbuf_string_steal(buf);
                  _elm_access_say(text);
                  eina_strbuf_free(buf);
               }
          }
     }
   else
     {
        _current_item_state_change
          (obj, MULTIBUTTONENTRY_BUTTON_STATE_DEFAULT);
        if (elm_widget_focus_get(obj) && sd->editable)
          elm_object_focus_set(sd->entry, EINA_TRUE);
     }
}

static void
_on_item_clicked(void *data,
                   Evas_Object *obj EINA_UNUSED,
                   const char *emission EINA_UNUSED,
                   const char *source EINA_UNUSED)
{
   Elm_Object_Item *eo_it = data;

   ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_it, it);
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(WIDGET(it), sd);

   if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
     return;

   _item_select(WIDGET(it), it);

   if (sd->selected_it)
     evas_object_smart_callback_call(VIEW(it), SIG_ITEM_CLICKED, eo_it);
}

static void
_on_item_deleted(void *data,
                   Evas_Object *obj,
                   const char *emission EINA_UNUSED,
                   const char *source EINA_UNUSED)
{
   Eina_List *l;
   Elm_Object_Item *eo_it = data;
   Elm_Object_Item *eo_temp_it;

   ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_it, it);
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(WIDGET(it), sd);
   // change the current
   EINA_LIST_FOREACH(sd->items, l, eo_temp_it)
     {
        ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_temp_it, temp_it);
        if (VIEW(temp_it) == obj)
          {
             eo_do(eo_temp_it, elm_wdg_item_del());
             break;
          }
     }
}

static Eina_Bool
_long_press_cb(void *data)
{
   Elm_Multibuttonentry_Item_Data *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   sd->longpress_timer = NULL;

   evas_object_smart_callback_call(WIDGET(it), SIG_ITEM_LONGPRESSED, EO_OBJ(it));

   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down_cb(void  *data,
               Evas *evas EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info)
{
   Elm_Multibuttonentry_Item_Data *it = data;
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(WIDGET(it), sd);

   if (ev->button != 1) return;

   ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
      (_elm_config->longpress_timeout, _long_press_cb, it);
}

static void
_mouse_up_cb(void *data,
             Evas *evas EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Elm_Multibuttonentry_Item_Data *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(WIDGET(it), sd);

   ELM_SAFE_FREE(sd->longpress_timer, ecore_timer_del);
}

EOLIAN static void
_elm_multibuttonentry_item_elm_widget_item_signal_emit(Eo *eo_item EINA_UNUSED,
                                                       Elm_Multibuttonentry_Item_Data *item,
                                                       const char *emission,
                                                       const char *source)
{
   elm_layout_signal_emit(VIEW(item), emission, source);
}

EOLIAN static void
_elm_multibuttonentry_item_elm_widget_item_part_text_set(Eo *eo_item EINA_UNUSED,
                                                    Elm_Multibuttonentry_Item_Data *item,
                                                    const char *part,
                                                    const char *label)
{
   const char *dest_part = NULL;

   if (!part || !strcmp(part, "elm.text"))
     dest_part = "elm.btn.text";
   else
     dest_part = part;

   edje_object_part_text_escaped_set(elm_layout_edje_get(VIEW(item)), dest_part, label);
}

EOLIAN static const char *
_elm_multibuttonentry_item_elm_widget_item_part_text_get(Eo *eo_item EINA_UNUSED,
                                                    Elm_Multibuttonentry_Item_Data *item,
                                                    const char *part)
{
   const char *src_part = NULL;

   if (!part || !strcmp(part, "elm.text"))
     src_part = "elm.btn.text";
   else
     src_part = part;

   return edje_object_part_text_get(elm_layout_edje_get(VIEW(item)), src_part);
}

EOLIAN static void
_elm_multibuttonentry_item_eo_base_destructor(Eo *eo_it,
                                              Elm_Multibuttonentry_Item_Data *it)
{
   _item_del(it);

   eo_do_super(eo_it, ELM_MULTIBUTTONENTRY_ITEM_CLASS, eo_destructor());
}

static void
_access_multibuttonentry_label_register(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);
   Evas_Object *po;

   po = (Evas_Object *)edje_object_part_object_get(sd->label, "elm.text");
   if (is_access)
     {
        Evas_Object *ao;
        ao = _elm_access_edje_object_part_object_register
                            (obj, sd->label, "elm.text");
        _elm_access_text_set(_elm_access_info_get(ao),
                             ELM_ACCESS_TYPE, E_("multi button entry label"));
     }
   else
     _elm_access_edje_object_part_object_unregister
                      (obj, sd->label, "elm.text");

   evas_object_pass_events_set(po, !is_access);
   evas_object_propagate_events_set(sd->label, !is_access);
}

static void
_access_multibuttonentry_item_register(Evas_Object *obj,
                                       Elm_Object_Item *eo_item,
                                       Eina_Bool is_access)
{
   ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
   if (is_access)
     {
        Evas_Object *ao;
        ao = _elm_access_edje_object_part_object_register
                      (obj, elm_layout_edje_get(VIEW(item)), "elm.btn.text");
        _elm_access_text_set(_elm_access_info_get(ao),
                             ELM_ACCESS_TYPE, E_("multi button entry item"));
     }
   else
     _elm_access_edje_object_part_object_unregister
                (obj, elm_layout_edje_get(VIEW(item)), "elm.btn.text");

   /* cannot read item because mouse-in event is delivered to
      the multibuttonentry resize_obj which is registered as an access
      object, so the mouse-in event should be blocked here */
   evas_object_propagate_events_set(VIEW(item), !is_access);
}

EOLIAN static void
_elm_multibuttonentry_item_eo_base_constructor(Eo *eo_item, Elm_Multibuttonentry_Item_Data *item)
{
   eo_do_super(eo_item, ELM_MULTIBUTTONENTRY_ITEM_CLASS, eo_constructor());
   item->base = eo_data_scope_get(eo_item, ELM_WIDGET_ITEM_CLASS);
}

static Elm_Object_Item *
_item_new(Elm_Multibuttonentry_Data *sd,
                 const char *str,
                 Multibuttonentry_Pos pos,
                 Elm_Object_Item *eo_reference,
                 Evas_Smart_Cb func,
                 void *data)
{
   Eina_List *l;
   Eo *eo_item;
   Elm_Multibuttonentry_Item_Filter *item_filter;
   Elm_Multibuttonentry_Item_Data *reference = eo_reference?
      eo_data_scope_get(eo_reference, ELM_MULTIBUTTONENTRY_ITEM_CLASS):
      NULL;
   Evas_Object *obj;
   int minw, minh, boxw;

   obj = sd->parent;

   if (!str) return NULL;

   EINA_LIST_FOREACH(sd->filter_list, l, item_filter)
     {
        if (!(item_filter->callback_func(obj, str, data, item_filter->data)))
          return NULL;
     }

   eo_item = eo_add(ELM_MULTIBUTTONENTRY_ITEM_CLASS, obj);
   if (!eo_item)
     return NULL;
   WIDGET_ITEM_DATA_SET(eo_item, data);

   ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
   VIEW(item) = elm_layout_add(obj);

   if (!elm_layout_theme_set
       (VIEW(item), "multibuttonentry", "btn", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   elm_object_part_text_set(VIEW(item), "elm.btn.text", str);

   //entry is cleared when text is made to button
   elm_object_text_set(sd->entry, "");

   elm_layout_signal_callback_add
     (VIEW(item), "mouse,clicked,1", "*", _on_item_clicked, EO_OBJ(item));
   elm_layout_signal_callback_add
     (VIEW(item), "elm,deleted", "elm", _on_item_deleted, EO_OBJ(item));
   evas_object_event_callback_add
      (VIEW(item),
       EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, item);
   evas_object_event_callback_add
      (VIEW(item),
       EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, item);

   evas_object_show(VIEW(item));

   evas_object_smart_calculate(VIEW(item));
   evas_object_size_hint_min_get(VIEW(item), &minw, &minh);
   evas_object_geometry_get(sd->box, NULL, NULL, &boxw, NULL);

   if (sd->w_box && minw > boxw)
     {
        elm_coords_finger_size_adjust(1, &boxw, 1, &minh);
        evas_object_size_hint_min_set(VIEW(item), boxw, minh);
        evas_object_resize(VIEW(item), boxw, minh);
     }

   elm_object_focus_allow_set(VIEW(item), EINA_TRUE);

   item->visible = EINA_TRUE;

   // ACCESS
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     {
        const char *text;
        Eina_Strbuf *buf;
        buf = eina_strbuf_new();

        eina_strbuf_append_printf(buf,
          "multi button entry item %s is added",
          edje_object_part_text_get(elm_layout_edje_get(VIEW(item)), "elm.btn.text"));

        text = (const char*)eina_strbuf_string_steal(buf);
        _elm_access_say(text);
        eina_strbuf_free(buf);

        _access_multibuttonentry_item_register(obj, eo_item, EINA_TRUE);
     }

   if (func)
     {
        item->func = func;
     }

   switch (pos)
     {
      case MULTIBUTTONENTRY_POS_START:
        sd->items = eina_list_prepend(sd->items, eo_item);
        if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
          {
             elm_widget_sub_object_add(obj, VIEW(item));
             _shrink_mode_set(obj, EINA_TRUE);
          }
        else
          {
             if (sd->label)
               elm_box_pack_after(sd->box, VIEW(item), sd->label);
             else
               elm_box_pack_start(sd->box, VIEW(item));
             if (sd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
               _visual_guide_text_set(obj);
          }
        break;

      case MULTIBUTTONENTRY_POS_END:
        sd->items = eina_list_append(sd->items, eo_item);
        if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
          {
             elm_widget_sub_object_add(obj, VIEW(item));
             evas_object_hide(VIEW(item));
          }
        else
          {
             if (sd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
               _visual_guide_text_set(obj);

             if (sd->editable)
               elm_box_pack_before(sd->box, VIEW(item), sd->entry);
             else
               elm_box_pack_end(sd->box, VIEW(item));
          }
        break;

      case MULTIBUTTONENTRY_POS_BEFORE:
        if (eo_reference)
          sd->items = eina_list_prepend_relative(sd->items, eo_item, eo_reference);
        else
          sd->items = eina_list_append(sd->items, eo_item);
        if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
          {
             elm_widget_sub_object_add(obj, VIEW(item));
             evas_object_hide(VIEW(item));
             _shrink_mode_set(obj, EINA_TRUE);
          }
        else
          {
             if (eo_reference)
               elm_box_pack_before(sd->box, VIEW(item), reference->button);
             else
               {
                  if (sd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
                    _visual_guide_text_set(obj);
                  if (sd->editable)
                    {
                       elm_box_pack_before(sd->box, VIEW(item), sd->entry);
                    }
                  else
                    elm_box_pack_end(sd->box, VIEW(item));
               }
          }
        break;

      case MULTIBUTTONENTRY_POS_AFTER:
        if (eo_reference)
          sd->items = eina_list_append_relative(sd->items, eo_item, eo_reference);
        else
          sd->items = eina_list_append(sd->items, eo_item);
        if (sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK)
          {
             elm_widget_sub_object_add(obj, VIEW(item));
             _shrink_mode_set(obj, EINA_TRUE);
          }
        else
          {
             if (eo_reference)
               elm_box_pack_after(sd->box, VIEW(item), reference->button);
             else
               {
                  if (sd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
                    _visual_guide_text_set(obj);
                  if (sd->editable)
                    {
                       elm_box_pack_before(sd->box, VIEW(item), sd->entry);
                    }
                  else
                    elm_box_pack_end(sd->box, VIEW(item));
               }
          }
        break;

      default:
        break;
     }
   evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, eo_item);

   return eo_item;
}

//FIXME: having an empty event handling function and reacting on Evas
//events on specific objects is crazy, someone should fix that.
EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_widget_event(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd EINA_UNUSED, Evas_Object *src, Evas_Callback_Type type, void *ev)
{
   (void)src;
   (void)type;
   (void)ev;

   // ACCESS
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON) return EINA_FALSE;

   return EINA_TRUE;
}

EOLIAN static void
_elm_multibuttonentry_elm_layout_sizing_eval(Eo *obj, Elm_Multibuttonentry_Data *sd EINA_UNUSED)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc
	      (wd->resize_obj, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_mouse_clicked_signal_cb(void *data EINA_UNUSED,
                         Evas_Object *obj,
                         const char *emission EINA_UNUSED,
                         const char *source EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_CLICKED, NULL);
}

static void
_box_resize_cb(void *data,
               Evas *evas EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event EINA_UNUSED)
{
   Evas_Coord w, h, mnw, mnh;
   Eina_List *l;
   Elm_Object_Item *eo_it;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);

   evas_object_geometry_get(sd->box, NULL, NULL, &w, &h);

   if (sd->h_box < h)
     evas_object_smart_callback_call(sd->parent, SIG_EXPANDED, NULL);
   else if (sd->h_box > h)
     evas_object_smart_callback_call(sd->parent, SIG_CONTRACTED, NULL);

   if (sd->w_box && sd->w_box != w)
     {
        if (sd->items)
          {

             EINA_LIST_FOREACH (sd->items, l, eo_it)
               {
                  ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_it, it);

                  elm_layout_sizing_eval(VIEW(it));
                  evas_object_smart_calculate(VIEW(it));

                  evas_object_size_hint_min_get(VIEW(it), &mnw, &mnh);

				  if (mnw > w)
                    {
                       mnw = w;
                       evas_object_size_hint_min_set(VIEW(it), mnw, mnh);
                       evas_object_resize(VIEW(it), mnw, mnh);
                    }
               }
          }
     }
   if (!elm_object_focus_get(data) && !evas_object_visible_get(sd->entry))
     {
        _shrink_mode_set(data, EINA_TRUE);
     }

   sd->w_box = w;
   sd->h_box = h;
}

static void
_entry_resize_cb(void *data,
                 Evas *e EINA_UNUSED,
                 Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);
   Evas_Coord en_x, en_y, en_w, en_h;

   evas_object_geometry_get(sd->entry, &en_x, &en_y, &en_w, &en_h);

   if (elm_widget_focus_get(sd->parent))
     elm_widget_show_region_set(sd->entry, en_x, en_y, en_w, en_h, EINA_TRUE);
}

static void
_entry_changed_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);
   const char *str;

   str = elm_object_text_get(sd->entry);
   sd->n_str = str ? strlen(str) : 0;
}

static void
_entry_focus_in_cb(void *data,
                   Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   Elm_Multibuttonentry_Item_Data *item = NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);

   if (sd->selected_it)
     {
        item = sd->selected_it;
        elm_object_focus_set(sd->entry, EINA_FALSE);
        evas_object_focus_set(VIEW(item), EINA_TRUE);
     }
}

static void
_entry_focus_out_cb(void *data,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);
   const char *str;

   str = elm_object_text_get(sd->entry);
   if (str && str[0])
     _item_new(sd, str, MULTIBUTTONENTRY_POS_END, NULL, NULL, NULL);
}

static void
_entry_clicked_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);

   _current_item_state_change(sd->parent, MULTIBUTTONENTRY_BUTTON_STATE_DEFAULT);
   elm_object_focus_set(sd->entry, EINA_TRUE);
}

static void
_layout_key_down_cb(void *data,
                  Evas *e EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;

   if (!sd->box) return;

   if (sd->last_it_select)
     {
        if (sd->selected_it &&
            ((!strcmp(ev->key, "BackSpace")) ||
             (!strcmp(ev->key, "Delete"))))
          {
             Elm_Multibuttonentry_Item_Data *item = sd->selected_it;
             if (item && sd->editable)
               {
                  eo_do(EO_OBJ(item), elm_wdg_item_del());
                  elm_object_focus_set(sd->entry, EINA_TRUE);
               }
          }
        else if (((!sd->selected_it && (sd->n_str == 0) &&
                   (!strcmp(ev->key, "BackSpace"))) ||
                  (!strcmp(ev->key, "Delete"))))
          {
             Elm_Object_Item *eo_item = eina_list_data_get(eina_list_last(sd->items));
             if (eo_item)
               {
                  ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
                  _item_select(sd->parent, item);
               }
          }
     }
   else
     sd->last_it_select = EINA_TRUE;
}

static void
_entry_key_down_cb(void *data,
                   Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED,
                   void *event_info)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);
   Evas_Event_Key_Down *ev = (Evas_Event_Key_Down *)event_info;

   if (sd->n_str == 1 &&
       (!strcmp(ev->key, "BackSpace") || !strcmp(ev->key, "Delete")))
     sd->last_it_select = EINA_FALSE;
}

static void
_entry_key_up_cb(void *data,
                 Evas *e EINA_UNUSED,
                 Evas_Object *obj EINA_UNUSED,
                 void *event_info)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(data, sd);
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;
   const char *str;

   if (!sd->box) return;

   str = elm_object_text_get(sd->entry);

   if (strlen(str) &&
       (!strcmp(ev->key, "KP_Enter") || !strcmp(ev->key, "Return")))
     {
        _item_new(sd, str, MULTIBUTTONENTRY_POS_END, NULL, NULL, NULL);
        sd->n_str = 0;
     }
}

static void
_callbacks_register(Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   elm_layout_signal_callback_add
     (obj, "mouse,clicked,1", "*", _mouse_clicked_signal_cb, NULL);

   evas_object_event_callback_add
     (wd->resize_obj, EVAS_CALLBACK_KEY_DOWN,
     _layout_key_down_cb, obj);

   evas_object_event_callback_add
     (sd->box, EVAS_CALLBACK_RESIZE, _box_resize_cb, obj);

   evas_object_event_callback_add
     (sd->entry, EVAS_CALLBACK_KEY_UP, _entry_key_up_cb, obj);
   evas_object_event_callback_add
     (sd->entry, EVAS_CALLBACK_KEY_DOWN, _entry_key_down_cb, obj);
   evas_object_event_callback_add
     (sd->entry, EVAS_CALLBACK_RESIZE, _entry_resize_cb, obj);
   evas_object_smart_callback_add
     (sd->entry, "changed", _entry_changed_cb, obj);
   evas_object_smart_callback_add
     (sd->entry, "focused", _entry_focus_in_cb, obj);
   evas_object_smart_callback_add
     (sd->entry, "unfocused", _entry_focus_out_cb, obj);
   evas_object_smart_callback_add
     (sd->entry, "clicked", _entry_clicked_cb, obj);
}

static void
_label_set(Evas_Object *obj,
           const char *str)
{
   Evas_Coord width, height;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   if (!str) return;

   eina_stringshare_replace(&sd->label_str, str);

   evas_object_size_hint_min_set(sd->label, 0, 0);
   evas_object_resize(sd->label, 0, 0);
   edje_object_part_text_escaped_set(sd->label, "elm.text", str);

   if (!strlen(str))
     {
        edje_object_signal_emit(sd->label, "elm,mbe,clear_text", "elm");
        edje_object_size_min_calc(sd->label, &width, &height);
     }
   else
     {
        edje_object_signal_emit(sd->label, "elm,mbe,set_text", "elm");
        edje_object_size_min_calc(sd->label, &width, &height);
     }

   elm_coords_finger_size_adjust(1, &width, 1, &height);
   evas_object_size_hint_min_set(sd->label, width, height);
   evas_object_show(sd->label);
   _view_update(sd);
}

static void
_guide_text_set(Evas_Object *obj,
                const char *str)
{
   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   if (!str) return;

   eina_stringshare_replace(&sd->guide_text_str, str);
   if (sd->guide_text == NULL)
     sd->guide_text = edje_object_add(evas_object_evas_get(obj));

   if (sd->guide_text)
     {
        elm_widget_theme_object_set(obj, sd->guide_text, "multibuttonentry",
                                    "guidetext", elm_widget_style_get(obj));
        evas_object_size_hint_weight_set
          (sd->guide_text, 0.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set
          (sd->guide_text, EVAS_HINT_FILL, EVAS_HINT_FILL);
        edje_object_part_text_escaped_set(sd->guide_text, "elm.text", str);
        _view_update(sd);
     }
}

static Elm_Multibuttonentry_Item_Filter *
_filter_new(Elm_Multibuttonentry_Item_Filter_Cb func,
            void *data)
{
   Elm_Multibuttonentry_Item_Filter *item_filter =
     ELM_NEW(Elm_Multibuttonentry_Item_Filter);
   if (!item_filter) return NULL;

   item_filter->callback_func = func;
   item_filter->data = data;

   return item_filter;
}

static void
_filter_free(Elm_Multibuttonentry_Item_Filter *item_filter)
{
   free(item_filter);
}

static Eina_Bool
_box_min_size_calculate(Evas_Object *box,
                        Evas_Object_Box_Data *priv,
                        int *line_height,
                        void *data EINA_UNUSED)
{
   Evas_Coord mnw, mnh, w, minw, minh = 0, linew = 0, lineh = 0;
   int line_num;
   Eina_List *l, *l_next;
   Evas_Object_Box_Option *opt;

   evas_object_geometry_get(box, NULL, NULL, &w, NULL);
   evas_object_size_hint_min_get(box, &minw, NULL);

   if (!w) return EINA_FALSE;

   line_num = 1;
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        evas_object_size_hint_min_get(opt->obj, &mnw, &mnh);

        linew += mnw;
        if (lineh < mnh) lineh = mnh;

        if (linew > w)
          {
             linew = mnw;
             line_num++;

             l_next = eina_list_next(l);
             opt = eina_list_data_get(l_next);
             if (l_next && opt && opt->obj &&
                 eo_isa(opt->obj, ELM_ENTRY_CLASS))
               {
                  linew = 0;
                  line_num++;
               }
          }
     }
   minh = lineh * line_num;

   evas_object_size_hint_min_set(box, minw, minh);
   *line_height = lineh;

   return EINA_TRUE;
}

static void
_box_layout_cb(Evas_Object *o,
               Evas_Object_Box_Data *priv,
               void *data)
{
   Evas_Coord x, y, w, h, xx, yy;
   Evas_Coord minw, minh, linew = 0, lineh = 0;
   Evas_Object_Box_Option *opt;
   const Eina_List *l, *l_next;
   Evas_Object *obj;
   double ax, ay;

   if (!_box_min_size_calculate(o, priv, &lineh, data)) return;

   evas_object_geometry_get(o, &x, &y, &w, &h);

   evas_object_size_hint_min_get(o, &minw, &minh);
   evas_object_size_hint_align_get(o, &ax, &ay);
   if (w < minw)
     {
        x = x + ((w - minw) * (1.0 - ax));
        w = minw;
     }
   if (h < minh)
     {
        y = y + ((h - minh) * (1.0 - ay));
        h = minh;
     }

   xx = x;
   yy = y;

   EINA_LIST_FOREACH_SAFE(priv->children, l, l_next, opt)
     {
        Evas_Coord mnw, mnh;
        Evas_Coord ww, hh, ow, oh;
        double wx, wy;
        int fw, fh;

        obj = opt->obj;
        evas_object_size_hint_align_get(obj, &ax, &ay);
        evas_object_size_hint_weight_get(obj, &wx, &wy);
        evas_object_size_hint_min_get(obj, &mnw, &mnh);

        fw = fh = EINA_FALSE;
        if (ax == -1.0) {fw = 1; ax = 0.5; }
        if (ay == -1.0) {fh = 1; ay = 0.5; }

        ww = mnw;
        if (wx)
          {
             if (ww <= w - linew) ww = w - linew;
             else ww = w;
          }
        hh = lineh;

        ow = mnw;
        if (fw) ow = ww;
        oh = mnh;
        if (fh) oh = hh;

        linew += ww;
        if (linew > w && l != priv->children)
          {
             xx = x;
             yy += hh;
             linew = ww;
          }

        evas_object_move(obj,
                         xx + (Evas_Coord)(((double)(ww - ow)) * ax),
                         yy + (Evas_Coord)(((double)(hh - oh)) * ay));
        evas_object_resize(obj, ow, oh);

        xx += ww;

        if (linew > w)
          {
             opt = eina_list_data_get(l_next);
             if (opt && opt->obj && eo_isa(opt->obj, ELM_ENTRY_CLASS))
               {
                  xx = x;
                  yy += hh;
                  linew = 0;
               }
          }
     }
}

static void
_view_init(Evas_Object *obj, Elm_Multibuttonentry_Data *sd)
{
   sd->box = elm_box_add(obj);

   if (!sd->box) return;
   elm_box_layout_set(sd->box, _box_layout_cb, obj, NULL);
   elm_box_homogeneous_set(sd->box, EINA_FALSE);
   elm_layout_content_set(obj, "box.swallow", sd->box);

   sd->label = edje_object_add(evas_object_evas_get(obj));
   if (!sd->label) return;
   elm_widget_theme_object_set
     (obj, sd->label, "multibuttonentry", "label",
     elm_widget_style_get(obj));
   _label_set(obj, "");
   elm_widget_sub_object_add(obj, sd->label);

   // ACCESS
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     _access_multibuttonentry_label_register(obj, EINA_TRUE);

   sd->entry = elm_entry_add(obj);
   if (!sd->entry) return;
   elm_entry_single_line_set(sd->entry, EINA_TRUE);
   elm_object_text_set(sd->entry, "");
   elm_entry_input_panel_enabled_set(sd->entry, EINA_FALSE);
   evas_object_size_hint_min_set(sd->entry, MIN_W_ENTRY, 0);
   evas_object_size_hint_weight_set
     (sd->entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sd->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(sd->box, sd->entry);
   evas_object_show(sd->entry);

   sd->view_state = MULTIBUTTONENTRY_VIEW_ENTRY;

     {
        Evas_Coord button_min_width = 0, button_min_height = 0;

        sd->end = edje_object_add(evas_object_evas_get(obj));
        if (!sd->end) return;
        elm_widget_theme_object_set
          (obj, sd->end, "multibuttonentry", "closedbutton",
          elm_widget_style_get(obj));

        edje_object_size_min_calc(sd->end, &button_min_width, &button_min_height);
        elm_coords_finger_size_adjust(1, &button_min_width, 1, &button_min_height);
        evas_object_size_hint_min_set(sd->end, button_min_width, button_min_height);
        elm_widget_sub_object_add(obj, sd->end);
     }
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_layout_text_set(Eo *obj, Elm_Multibuttonentry_Data *sd EINA_UNUSED, const char *part, const char *label)
{
   Eina_Bool int_ret = EINA_FALSE;

   if (!part || !strcmp(part, "default"))
     {
        if (label) _label_set(obj, label);
        int_ret = EINA_TRUE;
     }
   else if (!strcmp(part, "guide"))
     {
        if (label) _guide_text_set(obj, label);
        int_ret = EINA_TRUE;
     }
   else
     eo_do_super(obj, MY_CLASS, int_ret = elm_obj_layout_text_set(part, label));

   return int_ret;
}

EOLIAN static const char*
_elm_multibuttonentry_elm_layout_text_get(Eo *obj, Elm_Multibuttonentry_Data *sd, const char *part)
{
   const char *text = NULL;

   if (!part || !strcmp(part, "default"))
     {
        text = sd->label_str;
     }
   else if (!strcmp(part, "guide"))
     {
        text = sd->guide_text_str;
     }
   else
     eo_do_super(obj, MY_CLASS, text = elm_obj_layout_text_get(part));

   return text;
}

static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf;
   Eina_List *l = NULL;
   Elm_Object_Item *eo_item;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   if (sd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
     {
        if (sd->guide_text_str) return strdup(sd->guide_text_str);
        return NULL;
     }

   buf = eina_strbuf_new();

   if (sd->label_str) eina_strbuf_append(buf, sd->label_str);

   int invisible_its = 0;
   EINA_LIST_FOREACH (sd->items, l, eo_item)
     {
        ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
        if (item->visible)
          eina_strbuf_append_printf(buf, ", %s",
            edje_object_part_text_get(elm_layout_edje_get(VIEW(item)), "elm.btn.text"));
        else
           invisible_its++;
     }

   if (invisible_its)
     eina_strbuf_append_printf(buf, ", and %d more", invisible_its);

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

EOLIAN static void
_elm_multibuttonentry_evas_object_smart_add(Eo *obj, Elm_Multibuttonentry_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());
   elm_widget_sub_object_parent_add(obj);

   if (!elm_layout_theme_set
       (obj, "multibuttonentry", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   elm_widget_can_focus_set(obj, EINA_TRUE);

   priv->last_it_select = EINA_TRUE;
   priv->editable = EINA_TRUE;
   priv->parent = obj;
   priv->format_func = _format_count;

   _view_init(obj, priv);
   _callbacks_register(obj);

   // ACCESS
   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_TYPE, E_("multi button entry"));
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);
}

EOLIAN static void
_elm_multibuttonentry_evas_object_smart_del(Eo *obj, Elm_Multibuttonentry_Data *sd)
{
   Elm_Object_Item *eo_item;

   EINA_LIST_FREE(sd->items, eo_item)
     eo_del(eo_item);

   sd->items = NULL;

   sd->selected_it = NULL;

   eina_stringshare_del(sd->label_str);
   eina_stringshare_del(sd->guide_text_str);
   evas_object_del(sd->entry);
   evas_object_del(sd->label);
   evas_object_del(sd->guide_text);
   evas_object_del(sd->end);
   ecore_timer_del(sd->longpress_timer);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_del());
}

static Eina_Bool _elm_multibuttonentry_smart_focus_next_enable = EINA_FALSE;

EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_widget_focus_direction_manager_is(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_widget_focus_next_manager_is(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd EINA_UNUSED)
{
   return _elm_multibuttonentry_smart_focus_next_enable;
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_elm_widget_focus_next(Eo *obj, Elm_Multibuttonentry_Data *sd, Elm_Focus_Direction dir, Evas_Object **next)
{
   Eina_Bool int_ret = EINA_FALSE;

   Eina_List *items = NULL;
   Eina_List *l = NULL;
   Elm_Object_Item *eo_item;
   Evas_Object *ao;
   Evas_Object *po;

   if (!elm_widget_focus_get(obj))
     {
        *next = (Evas_Object *)obj;
        return EINA_TRUE;
     }

   if (sd->label)
     {
        po = (Evas_Object *)edje_object_part_object_get(sd->label, "elm.text");
        ao = evas_object_data_get(po, "_part_access_obj");
        int_ret = elm_widget_focus_get(ao);
        items = eina_list_append(items, ao);
     }

   EINA_LIST_FOREACH (sd->items, l, eo_item)
     {
        ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
        po = (Evas_Object *)edje_object_part_object_get
                           (elm_layout_edje_get(VIEW(item)), "elm.btn.text");
        ao = evas_object_data_get(po, "_part_access_obj");
        int_ret = int_ret || elm_widget_focus_get(ao);
        items = eina_list_append(items, ao);
     }

   if (sd->entry)
     {
        int_ret = int_ret || elm_widget_focus_get(sd->entry);
        /* elm_widget_list_focus_liset_next_get() check parent of item
           because parent sd->entry is not multibuttnentry but sd->box
           so append sd->box instead of sd->entry, is this proper? */
        items = eina_list_append(items, sd->box);
     }

   if (int_ret)
     return elm_widget_focus_list_next_get
              (obj, items, eina_list_data_get, dir, next);

   return EINA_FALSE;
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Eina_List *l;
   Elm_Object_Item *it;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN(obj, sd);

   /* label */
   _access_multibuttonentry_label_register(obj, is_access);

   /* buttons */
   EINA_LIST_FOREACH(sd->items, l, it)
     _access_multibuttonentry_item_register(obj, it, is_access);
}

EOLIAN static void
_elm_multibuttonentry_elm_widget_access(Eo *obj, Elm_Multibuttonentry_Data *sd EINA_UNUSED, Eina_Bool acs)
{
   _elm_multibuttonentry_smart_focus_next_enable = acs;
   _access_obj_process(obj, _elm_multibuttonentry_smart_focus_next_enable);
}

EAPI Evas_Object *
elm_multibuttonentry_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   return obj;
}

EOLIAN static void
_elm_multibuttonentry_eo_base_constructor(Eo *obj, Elm_Multibuttonentry_Data *sd EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks),
         elm_interface_atspi_accessible_role_set(ELM_ATSPI_ROLE_ENTRY));
}

EOLIAN static Evas_Object*
_elm_multibuttonentry_entry_get(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   return sd->entry;
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_expanded_get(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   return sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK ?
          EINA_FALSE : EINA_TRUE;
}

EOLIAN static void
_elm_multibuttonentry_format_function_set(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, Elm_Multibuttonentry_Format_Cb f_func, const void *data)
{
   sd->format_func = f_func;
   if (!sd->format_func) sd->format_func = _format_count;

   sd->format_func_data = data;

   _view_update(sd);
}

EOLIAN static void
_elm_multibuttonentry_expanded_set(Eo *obj, Elm_Multibuttonentry_Data *sd, Eina_Bool expanded)
{
   if (((sd->view_state == MULTIBUTTONENTRY_VIEW_SHRINK) ?
        EINA_FALSE : EINA_TRUE) == expanded) return;

   if (expanded)
     _shrink_mode_set(obj, EINA_FALSE);
   else
     _shrink_mode_set(obj, EINA_TRUE);
}

EOLIAN static void
_elm_multibuttonentry_editable_set(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, Eina_Bool editable)
{
   editable = !!editable;
   if (sd->editable == editable) return;
   sd->editable = editable;

   if (sd->editable)
     evas_object_show(sd->entry);
   else
     {
        elm_entry_input_panel_hide(sd->entry);
        evas_object_hide(sd->entry);
     }
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_editable_get(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   return sd->editable;
}

EOLIAN static Elm_Object_Item*
_elm_multibuttonentry_item_prepend(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, const char *label, Evas_Smart_Cb func, void *data)
{
   return _item_new(sd, label, MULTIBUTTONENTRY_POS_START, NULL, func, data);
}

EOLIAN static Elm_Object_Item*
_elm_multibuttonentry_item_append(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, const char *label, Evas_Smart_Cb func, void *data)
{
   return _item_new(sd, label, MULTIBUTTONENTRY_POS_END, NULL, func, data);
}

EOLIAN static Elm_Object_Item*
_elm_multibuttonentry_item_insert_before(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, Elm_Object_Item *before, const char *label, Evas_Smart_Cb func, void *data)
{
   return _item_new(sd, label, MULTIBUTTONENTRY_POS_BEFORE, before, func, data);
}

EOLIAN static Elm_Object_Item*
_elm_multibuttonentry_item_insert_after(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, Elm_Object_Item *after, const char *label, Evas_Smart_Cb func, void *data)
{
   return _item_new(sd, label, MULTIBUTTONENTRY_POS_AFTER, after, func, data);
}

EOLIAN static const Eina_List*
_elm_multibuttonentry_items_get(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   return sd->items;
}

EOLIAN static Elm_Object_Item*
_elm_multibuttonentry_first_item_get(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   return eina_list_data_get(sd->items);
}

EOLIAN static Elm_Object_Item*
_elm_multibuttonentry_last_item_get(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   return eina_list_data_get(eina_list_last(sd->items));
}

EOLIAN static Elm_Object_Item*
_elm_multibuttonentry_selected_item_get(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   return EO_OBJ(sd->selected_it);
}

EOLIAN static void
_elm_multibuttonentry_item_selected_set(Eo *eo_item EINA_UNUSED,
                                        Elm_Multibuttonentry_Item_Data *item,
                                        Eina_Bool selected)
{
   if (selected) _item_select(WIDGET(item), item);
   else _item_select(WIDGET(item), NULL);
}

EOLIAN static Eina_Bool
_elm_multibuttonentry_item_selected_get(Eo *eo_item,
                                        Elm_Multibuttonentry_Item_Data *item EINA_UNUSED)
{
   //TODO : To be implemented.
   if (!eo_item) return EINA_FALSE;

   return EINA_TRUE;
}

EOLIAN static void
_elm_multibuttonentry_clear(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd)
{
   Elm_Object_Item *eo_item;

   if (sd->items)
     {
        EINA_LIST_FREE(sd->items, eo_item)
          {
             ELM_MULTIBUTTONENTRY_ITEM_DATA_GET(eo_item, item);
             elm_box_unpack(sd->box, VIEW(item));
             eo_del(eo_item);
          }
        sd->items = NULL;
     }
   sd->selected_it = NULL;
   _view_update(sd);
}

EOLIAN static Elm_Object_Item *
_elm_multibuttonentry_item_prev_get(Eo *eo_it,
                                   Elm_Multibuttonentry_Item_Data *it)
{
   Eina_List *l;
   Elm_Object_Item *eo_temp_it;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN_VAL(WIDGET(it), sd, NULL);

   EINA_LIST_FOREACH(sd->items, l, eo_temp_it)
     {
        if (eo_temp_it == eo_it)
          {
             l = eina_list_prev(l);
             if (!l) return NULL;
             return eina_list_data_get(l);
          }
     }
   return NULL;
}

EOLIAN static Elm_Object_Item *
_elm_multibuttonentry_item_next_get(Eo *eo_it,
                                   Elm_Multibuttonentry_Item_Data *it)
{
   Eina_List *l;
   Elm_Object_Item *eo_temp_it;

   ELM_MULTIBUTTONENTRY_DATA_GET_OR_RETURN_VAL(WIDGET(it), sd, NULL);

   EINA_LIST_FOREACH(sd->items, l, eo_temp_it)
     {
        if (eo_temp_it == eo_it)
          {
             l = eina_list_next(l);
             if (!l) return NULL;
             return eina_list_data_get(l);
          }
     }
   return NULL;
}

EINA_DEPRECATED EAPI void *
elm_multibuttonentry_item_data_get(const Elm_Object_Item *it)
{
   return (void *)WIDGET_ITEM_DATA_GET(it);
}

EINA_DEPRECATED EAPI void
elm_multibuttonentry_item_data_set(Elm_Object_Item *it,
                                   void *data)
{
   WIDGET_ITEM_DATA_SET(it, data);
}

EOLIAN static void
_elm_multibuttonentry_item_filter_append(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, Elm_Multibuttonentry_Item_Filter_Cb func, void *data)
{
   Elm_Multibuttonentry_Item_Filter *new_item_filter = NULL;
   Elm_Multibuttonentry_Item_Filter *_item_filter = NULL;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(sd->filter_list, l, _item_filter)
     {
        if (_item_filter && ((_item_filter->callback_func == func)
                             && (_item_filter->data == data)))
          {
             INF("Already Registered this item filter!!!!\n");
             return;
          }
     }
   new_item_filter = _filter_new(func, data);
   if (!new_item_filter) return;

   sd->filter_list = eina_list_append(sd->filter_list, new_item_filter);
}

EOLIAN static void
_elm_multibuttonentry_item_filter_prepend(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, Elm_Multibuttonentry_Item_Filter_Cb func, void *data)
{
   Elm_Multibuttonentry_Item_Filter *new_item_filter = NULL;
   Elm_Multibuttonentry_Item_Filter *_item_filter = NULL;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(func);

   new_item_filter = _filter_new(func, data);
   if (!new_item_filter) return;

   EINA_LIST_FOREACH(sd->filter_list, l, _item_filter)
     {
        if (_item_filter && ((_item_filter->callback_func == func)
                             && (_item_filter->data == data)))
          {
             INF("Already Registered this item filter!!!!\n");
             _filter_free(new_item_filter);
             return;
          }
     }
   sd->filter_list = eina_list_prepend(sd->filter_list, new_item_filter);
}

EOLIAN static void
_elm_multibuttonentry_item_filter_remove(Eo *obj EINA_UNUSED, Elm_Multibuttonentry_Data *sd, Elm_Multibuttonentry_Item_Filter_Cb func, void *data)
{
   Eina_List *l;
   Elm_Multibuttonentry_Item_Filter *item_filter;

   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(sd->filter_list, l, item_filter)
     {
        if ((item_filter->callback_func == func)
            && ((!data) || (item_filter->data == data)))
          {
             sd->filter_list = eina_list_remove_list(sd->filter_list, l);
             _filter_free(item_filter);
             return;
          }
     }
}

static void
_elm_multibuttonentry_class_constructor(Eo_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);

   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
      _elm_multibuttonentry_smart_focus_next_enable = EINA_TRUE;
}

#include "elm_multibuttonentry_item.eo.c"
#include "elm_multibuttonentry.eo.c"
